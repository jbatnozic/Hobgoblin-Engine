// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Chunk_storage_handler.hpp>

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>

#include <thread>

namespace jbatnozic {
namespace gridgoblin {
namespace detail {

static constexpr auto LOG_ID = "GridGoblin";

#define CHUNK_AT_ID(_id_) \
    (_chunks[static_cast<hg::PZInteger>((_id_).y)][static_cast<hg::PZInteger>((_id_).x)])

ChunkStorageHandler::ChunkStorageHandler(const WorldConfig& aConfig)
    : _chunks{aConfig.chunkCountX, aConfig.chunkCountY}
    , _chunkWidth{aConfig.cellsPerChunkX}
    , _chunkHeight{aConfig.cellsPerChunkY}
    , _freeChunkLimit{aConfig.maxLoadedNonessentialChunks} {}

///////////////////////////////////////////////////////////////////////////
// MARK: DEPENDENCIES                                                    //
///////////////////////////////////////////////////////////////////////////

void ChunkStorageHandler::setChunkSpooler(ChunkSpoolerInterface* aChunkSpooler) {
    _chunkSpooler = aChunkSpooler;
}

void ChunkStorageHandler::setBinder(Binder* aBinder) {
    _binder = aBinder;
}

///////////////////////////////////////////////////////////////////////////
// MARK: ACTIVE AREAS                                                    //
///////////////////////////////////////////////////////////////////////////

ActiveArea ChunkStorageHandler::createNewActiveArea() {
    return ActiveArea{this};
}

///////////////////////////////////////////////////////////////////////////
// MARK: CYCLE                                                           //
///////////////////////////////////////////////////////////////////////////

void ChunkStorageHandler::update() {
    auto iter = _chunkControlBlocks.begin();
    for (; iter != _chunkControlBlocks.end(); iter = std::next(iter)) {
        auto& cb = iter->second;
        if (cb.requestHandle != nullptr && cb.requestHandle->isFinished()) {
            const auto id    = iter->first;
            auto       chunk = cb.requestHandle->takeChunk();
            if (chunk.has_value()) {
                _onChunkLoaded(id, std::move(*chunk));
            } else {
                _createDefaultChunk(id);
            }
            cb.requestHandle = nullptr;
        }
    }
}

void ChunkStorageHandler::prune() {
    HG_ASSERT(_chunkSpooler != nullptr);

    if (_freeChunks.size() <= hg::pztos(_freeChunkLimit)) {
        return;
    }

    do {
        const auto iter = _freeChunks.begin();
        const auto id   = iter->first;

        auto& chunk = CHUNK_AT_ID(id);
        HG_HARD_ASSERT(!chunk.isEmpty());
        _chunkSpooler->unloadChunk(id, std::move(chunk));
        chunk.makeEmpty();
        _chunksInGridCount -= 1;

        _freeChunks.erase(iter);
    } while (_freeChunks.size() > hg::pztos(_freeChunkLimit));

    HG_HARD_ASSERT(hg::pztos(_chunksInGridCount) <= _chunkControlBlocks.size() + _freeChunks.size());
}

///////////////////////////////////////////////////////////////////////////
// MARK: PRIVATE                                                         //
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
    int                 i = 0;
    while (!requestHandle->isFinished()) {
        if (stopwatch.getElapsedTime() >= std::chrono::milliseconds{10}) {
            stopwatch.restart();
            i += 1;
            HG_LOG_DEBUG(LOG_ID, "Blocking until chunk {} is loaded ({}ms elapsed so far).", id, i * 10);
        }
        std::this_thread::yield();
    }

    auto chunk = requestHandle->takeChunk();
    if (chunk.has_value()) {
        _onChunkLoaded(id, std::move(*chunk));
    } else {
        _createDefaultChunk(id);
    }

    if (iter == _chunkControlBlocks.end()) {
        _freeChunks.insert(std::make_pair(id, std::chrono::steady_clock::now()));
    }
}

void ChunkStorageHandler::_onChunkLoaded(ChunkId aChunkId, Chunk&& aChunk) {
    HG_HARD_ASSERT(!aChunk.isEmpty());

    auto& chunk = (CHUNK_AT_ID(aChunkId) = std::move(aChunk));
    _chunksInGridCount += 1;

    HG_ASSERT(_binder != nullptr);
    _binder->onChunkLoaded(aChunkId, chunk);
}

void ChunkStorageHandler::_createDefaultChunk(ChunkId aChunkId) {
    // Construct a chunk where all cells are in their default state
    // (floor, wall, etc. - all uninitialized).
    auto defaultChunk = Chunk{_chunkWidth, _chunkHeight};

    // Attach an extension if possible
    HG_ASSERT(_binder != nullptr);
    auto extension = _binder->createChunkExtension();
    if (extension != nullptr) {
        extension->init(aChunkId, defaultChunk);
    }
    defaultChunk.setExtension(std::move(extension));

    auto& chunk = (CHUNK_AT_ID(aChunkId) = std::move(defaultChunk));
    _chunksInGridCount += 1;

    _binder->onChunkCreated(aChunkId, chunk);
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

        // Consider if the chunk ID can be found in _chunkControlBlocks
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
                            if (cb.requestHandle->isFinished()) {
                                // Handle edge case where the request was finished but the owning
                                // active area moves before the chunk could be integrated
                                auto chunk = cb.requestHandle->takeChunk();
                                if (chunk.has_value()) {
                                    _onChunkLoaded(chunkId, std::move(*chunk));
                                } else {
                                    _createDefaultChunk(chunkId);
                                }
                            } else {
                                cb.requestHandle->cancel();
                            }
                        }
                        if (!CHUNK_AT_ID(chunkId).isEmpty()) {
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

        // Consider if the chunk is already loaded among the free chunks
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
            auto& cb = _chunkControlBlocks[chunkId];
            HG_HARD_ASSERT(_chunkControlBlocks.size() > 0);
            cb.usageCount = usageDelta;
            cbs.push_back(&cb);
            requests.push_back({chunkId, change.loadPriority, [this](ChunkId aChunkId) {
                                    if (_binder != nullptr) {
                                        _binder->onChunkReady(aChunkId);
                                    }
                                }});
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

///////////////////////////////////////////////////////////////////////////
// MARK: ITERATOR                                                        //
///////////////////////////////////////////////////////////////////////////

void ChunkStorageHandler::AvailableChunkIterator::advance() {
    if (_isEndIter) {
        return;
    }

    switch (_selector) {
    case SELECTOR_CB:
        while (true) {
            ++_cbIter;
            if (_cbIter == _cbMap.end()) {
                if (!_fcMap.empty()) {
                    _fcIter   = _fcMap.begin();
                    _selector = SELECTOR_FC;
                } else {
                    _isEndIter = true;
                }
                break;
            } else if (_cbIter->second.isChunkLoaded()) {
                break;
            }
        }
        break;

    case SELECTOR_FC:
        ++_fcIter;
        if (_fcIter == _fcMap.end()) {
            _isEndIter = true;
        }
        break;

    default:
        HG_UNREACHABLE("Invalid _selector value ({}).", (int)_selector);
    }
}

ChunkId ChunkStorageHandler::AvailableChunkIterator::dereference() const {
    HG_ASSERT(!_isEndIter);
    if (_isEndIter) {
        return {};
    }

    switch (_selector) {
    case SELECTOR_CB:
        return _cbIter->first;

    case SELECTOR_FC:
        return _fcIter->first;

    default:
        HG_UNREACHABLE("Invalid _selector value ({}).", (int)_selector);
    }
}

bool ChunkStorageHandler::AvailableChunkIterator::equals(const AvailableChunkIterator& aOther) const {
    if (&_cbMap == &aOther._cbMap && &_fcMap == &aOther._fcMap) {
        if (_isEndIter && aOther._isEndIter) {
            return true;
        }
        if (_isEndIter != aOther._isEndIter || _selector != aOther._selector) {
            return false;
        }
        switch (_selector) {
        case SELECTOR_CB:
            return _cbIter == aOther._cbIter;

        case SELECTOR_FC:
            return _fcIter == aOther._fcIter;

        default:
            HG_UNREACHABLE("Invalid _selector value ({}).", (int)_selector);
        }
    }
    return false;
}

ChunkStorageHandler::AvailableChunkIterator::AvailableChunkIterator(
    const decltype(_chunkControlBlocks)& aCbMap,
    const decltype(_freeChunks)&         aFcMap,
    bool                                 aIsEndIter)
    : _cbMap{aCbMap}
    , _cbIter{_cbMap.begin()}
    , _fcMap{aFcMap}
    , _fcIter{_fcMap.begin()}
    , _isEndIter{aIsEndIter} //
{
    if (!_isEndIter) {
        if (!_cbMap.empty()) {
            _selector = SELECTOR_CB;
            while (_selector == SELECTOR_CB && !_cbIter->second.isChunkLoaded()) {
                advance();
            }
            return;
        }
        if (!_fcMap.empty()) {
            _selector = SELECTOR_FC;
            return;
        }
        _isEndIter = true;
    }
}

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
