#pragma once

#include <GridWorld/Private/Chunk_storage_handler.hpp>

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>

#include <algorithm>
#include <thread>

namespace gridworld {
namespace detail {

static constexpr auto LOG_ID = "Griddy";

ChunkStorageHandler::ChunkStorageHandler(const WorldConfig& aConfig)
    : _chunks{aConfig.chunkCountX, aConfig.chunkCountY}
    , _chunkWidth{aConfig.cellsPerChunkX}
    , _chunkHeight{aConfig.cellsPerChunkY}
    , _freeChunkLimit{aConfig.maxLoadedNonessentialCells} {}

///////////////////////////////////////////////////////////////////////////
// DEPENDENCIES                                                          //
///////////////////////////////////////////////////////////////////////////

void ChunkStorageHandler::setChunkSpooler(ChunkSpoolerInterface* aChunkSpooler) {
    _chunkSpooler = aChunkSpooler;
}

void ChunkStorageHandler::setBinder(Binder* aBinder) {
    _binder = aBinder;
}

///////////////////////////////////////////////////////////////////////////
// ACTIVE AREAS                                                          //
///////////////////////////////////////////////////////////////////////////

ActiveArea ChunkStorageHandler::createNewActiveArea() {
    return ActiveArea{this};
}

///////////////////////////////////////////////////////////////////////////
// CYCLE                                                                 //
///////////////////////////////////////////////////////////////////////////

//! Collects all chunks that were loaded since the last call to `update()`
//! and makes them available.
void ChunkStorageHandler::update() {
    auto iter = _chunkControlBlocks.begin();
    for (; iter != _chunkControlBlocks.end(); iter = std::next(iter)) {
        auto& cb = iter->second;
        if (cb.requestHandle != nullptr && cb.requestHandle->isFinished()) {
            const auto id    = iter->first;
            auto       chunk = cb.requestHandle->takeChunk();
            if (chunk.has_value()) {
                _onChunkLoaded(id, std::move(*chunk), iter);
            } else {
                _createDefaultChunk(id);
            }
            // cb.requestHandle = nullptr; TODO

            if (_binder) {
                auto& chunk = _chunks[(hg::PZInteger)id.y][(hg::PZInteger)id.x];
                _binder->onChunkLoaded(id, &chunk); // TODO: temporary?
            }
        }
    }
}

void ChunkStorageHandler::prune() {
    HG_ASSERT(_chunkSpooler != nullptr);

    while (_freeChunks.size() > hg::pztos(_freeChunkLimit)) {
        const auto iter = _freeChunks.begin();
        const auto id   = iter->first;

        auto& chunk = _chunks[static_cast<hg::PZInteger>(id.y)][static_cast<hg::PZInteger>(id.x)];
        _chunkSpooler->unloadChunk(id, std::move(chunk));
        chunk.makeEmpty();

        _freeChunks.erase(iter);
    }
}

///////////////////////////////////////////////////////////////////////////
// PRIVATE                                                               //
///////////////////////////////////////////////////////////////////////////

/*
 * Note: Expect 10ms latency to read file from a HDD, and 1ms to read a file
 *       from an SSD (rough numbers but the orders of magnitude should be correct).
 */
void ChunkStorageHandler::_loadChunkImmediately(ChunkId aChunkId) {
    const auto id = aChunkId;

    HG_LOG_WARN(LOG_ID,
                "Requesting immediate load of chunk {}. "
                "Seeing this message too often likely means that the program is not well calibrated "
                "and that performance is suffering as a consequence.",
                id);

    std::shared_ptr<ChunkSpoolerInterface::RequestHandleInterface> requestHandle;

    static constexpr hg::PZInteger MOST_URGENT_PRIORITY = 0;

    const auto iter = _chunkControlBlocks.find(id);
    if (iter != _chunkControlBlocks.end()) {
        requestHandle = iter->second.requestHandle;
        (void)requestHandle->trySwapPriority(MOST_URGENT_PRIORITY);
    } else {
        HG_ASSERT(_chunkSpooler != nullptr);
        auto handles  = _chunkSpooler->loadChunks({
            ChunkSpoolerInterface::LoadRequest{id, MOST_URGENT_PRIORITY}
        });
        requestHandle = std::move(handles.at(0));
    }

    hg::util::Stopwatch stopwatch;
    for (int i = 0; !requestHandle->isFinished(); i += 1) {
        if (stopwatch.getElapsedTime() >= std::chrono::milliseconds{10}) {
            stopwatch.restart();
            HG_LOG_WARN(LOG_ID, "Blocking until chunk {} is loaded ({}ms elapsed so far).", id, i * 10);
        }
        std::this_thread::yield();
    }

    auto chunk = requestHandle->takeChunk();
    if (chunk.has_value()) {
        _onChunkLoaded(id, std::move(*chunk), iter);
    } else {
        _createDefaultChunk(id);
    }
}

void ChunkStorageHandler::_onChunkLoaded(
    ChunkId                                                aChunkId,
    Chunk&&                                                aChunk,
    std::optional<decltype(_chunkControlBlocks)::iterator> aControlBlockIterator)
//
{
    ChunkControlBlock* cb = nullptr;

    {
        auto iter = aControlBlockIterator.value_or(_chunkControlBlocks.find(aChunkId));
        if (iter != _chunkControlBlocks.end()) {
            cb = &(iter->second);
        } else {
            cb = &(_chunkControlBlocks[aChunkId]);
        }
    }

    HG_ASSERT(cb != nullptr);

    cb->requestHandle = nullptr;

    _chunks[static_cast<hg::PZInteger>(aChunkId.y)][static_cast<hg::PZInteger>(aChunkId.x)] =
        std::move(aChunk);
}

void ChunkStorageHandler::_createDefaultChunk(ChunkId aChunkId) {
    // TODO: temp impl
    _chunks[static_cast<hg::PZInteger>(aChunkId.y)][static_cast<hg::PZInteger>(aChunkId.x)] =
        Chunk{_chunkWidth, _chunkHeight};
}

void ChunkStorageHandler::_updateChunkUsage(
    const std::vector<detail::ChunkUsageChange>& aChunkUsageChanges)
//
{
    // Chunk control blocks and the associated chunk load requests which are to be passed
    // to the spooler. The vectors are to be kept in sync: the handle belonging to requests[N]
    // will be held by cbs[N].
    std::vector<ChunkControlBlock*>                 cbs;
    std::vector<ChunkSpoolerInterface::LoadRequest> requests;

    const auto predictedRequestCount = aChunkUsageChanges.size() / 2;
    cbs.reserve(predictedRequestCount);
    requests.reserve(predictedRequestCount);

    for (const auto change : aChunkUsageChanges) {
        const auto chunkId    = change.chunkId;
        const auto usageDelta = change.usageDelta;

        {
            const auto iter = _chunkControlBlocks.find(chunkId);
            if (iter != _chunkControlBlocks.end()) {
                auto& cb = iter->second;
                if (usageDelta > 0) {
                    cb.usageCount += usageDelta;
                    if (cb.requestHandle != nullptr) {
                        cb.requestHandle->tryBoostPriority(change.loadPriority);
                    }
                } else if (usageDelta < 0) {
                    HG_ASSERT(cb.usageCount <= -usageDelta);
                    if ((cb.usageCount += usageDelta) == 0) {
                        if (cb.requestHandle) {
                            cb.requestHandle->cancel();
                        }
                        if (!_chunks[chunkId.y][chunkId.x]
                                 .isEmpty()) { // TODO: make method for chunkAtId()
                            _freeChunks.insert(
                                std::make_pair(chunkId, std::chrono::steady_clock::now()));
                        }
                        _chunkControlBlocks.erase(iter);
                    }
                } else {
                    HG_LOG_WARN(LOG_ID, "Usage delta of 0 received for chunk {}.", chunkId);
                }
                continue;
            }
        }

        {
            const auto iter = _freeChunks.find(chunkId);
            if (iter != _freeChunks.end()) {
                HG_ASSERT(usageDelta >= 0);
                if (usageDelta > 0) {
                    auto& cb      = _chunkControlBlocks[chunkId];
                    cb.usageCount = usageDelta;
                    _freeChunks.erase(iter);
                } else {
                    HG_LOG_WARN(LOG_ID,
                                "Usage delta of 0 or less ({}) received for free chunk {}.",
                                usageDelta,
                                chunkId);
                }
                continue;
            }
        }

        // At this point, the chunk wasn't found in any collection - if the delta is correct,
        // request a load
        HG_ASSERT(usageDelta >= 0);
        if (usageDelta > 0) {
            auto& cb      = _chunkControlBlocks[chunkId];
            cb.usageCount = usageDelta;
            cbs.push_back(&cb);
            requests.push_back({chunkId, change.loadPriority});
        } else {
            HG_LOG_WARN(LOG_ID,
                        "Usage delta of 0 or less ({}) received for chunk {}.",
                        usageDelta,
                        chunkId);
        }
    } // end for

    HG_HARD_ASSERT(cbs.size() == requests.size());
    if (!requests.empty()) {
        HG_ASSERT(_chunkSpooler != nullptr);
        auto handles = _chunkSpooler->loadChunks(requests);
        HG_ASSERT(handles.size() == cbs.size());
        for (std::size_t i = 0; i < handles.size(); i += 1) {
            cbs[i]->requestHandle = std::move(handles[i]);
        }
    }
}

} // namespace detail
} // namespace gridworld
