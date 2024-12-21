#pragma once

#include <Hobgoblin/Common.hpp>

#include <GridWorld/Model/Chunk.hpp>
#include <GridWorld/Model/Chunk_id.hpp>
#include <GridWorld/Private/Chunk_spooler_interface.hpp>
#include <GridWorld/World/Active_area.hpp>
#include <GridWorld/World/Binder.hpp>
#include <GridWorld/World/World_config.hpp>

#include <Hobgoblin/Utility/Grids.hpp>
#include <Hobgoblin/Utility/Value_sorted_map.hpp>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>

namespace gridworld {

namespace hg = jbatnozic::hobgoblin;

class Binder;

namespace detail {

// TODO: move definitions somewhere else

#if defined(__GNUC__) && !defined(__clang__)
#define HG_LIKELY_CONDITION(...)   __builtin_expect(!!(__VA_ARGS__), 1)
#define HG_UNLIKELY_CONDITION(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
#define HG_LIKELY_CONDITION(...)   __VA_ARGS__
#define HG_UNLIKELY_CONDITION(...) __VA_ARGS__
#endif

#define HG_LIKELY_BRANCH   [[likely]]
#define HG_UNLIKELY_BRANCH [[unlikely]]

HG_DECLARE_TAG_TYPE(LOAD_IF_MISSING);

//! This class handles the storage of chunks both in RAM and in on-disk caches.
class ChunkStorageHandler {
private:
    using Self = ChunkStorageHandler;

public:
    ChunkStorageHandler(const WorldConfig& aConfig);

    ///////////////////////////////////////////////////////////////////////////
    // DEPENDENCIES                                                          //
    ///////////////////////////////////////////////////////////////////////////
    
    void setChunkSpooler(ChunkSpoolerInterface* aChunkSpooler);

    void setBinder(Binder* aBinder);

    ///////////////////////////////////////////////////////////////////////////
    // ACTIVE AREAS                                                          //
    ///////////////////////////////////////////////////////////////////////////

    ActiveArea createNewActiveArea();

    ///////////////////////////////////////////////////////////////////////////
    // CYCLE                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    //! Collects all chunks that were loaded since the last call to `update()`
    //! and makes them available.
    void update();

    void prune();

    ///////////////////////////////////////////////////////////////////////////
    // CHUNK GETTERS                                                         //
    ///////////////////////////////////////////////////////////////////////////

    //! Returns the number of chunks along the X axis.
    hg::PZInteger getChunkCountX() const {
        return _chunks.getWidth();
    }

    //! Returns the number of chunks along the Y axis.
    hg::PZInteger getChunkCountY() const {
        return _chunks.getHeight();
    }

    //! Returns a mutable reference to the chunk with the given ID WITHOUT CHECKING BOUNDS.
    //! The chunk will be loaded if it isn't already, and will remain loaded until the next
    //! call to `prune()`.
    Chunk& getChunkAtIdUnchecked(ChunkId aChunkId, LOAD_IF_MISSING_Tag) {
        auto& chunk =
            _chunks[static_cast<hg::PZInteger>(aChunkId.y)][static_cast<hg::PZInteger>(aChunkId.x)];
        if (HG_LIKELY_CONDITION(!chunk.isEmpty())) {
            HG_LIKELY_BRANCH;
            return chunk;
        } else {
            HG_UNLIKELY_BRANCH;
            _loadChunkImmediately(aChunkId);
            return chunk;
        }
    }

    //! Returns a const reference to the chunk with the given ID WITHOUT CHECKING BOUNDS.
    //! The chunk will be loaded if it isn't already, and will remain loaded until the next
    //! call to `prune()`.
    const Chunk& getChunkAtIdUnchecked(ChunkId aChunkId, LOAD_IF_MISSING_Tag) const {
        return const_cast<Self*>(this)->getChunkAtIdUnchecked(aChunkId, LOAD_IF_MISSING);
    }

    //! Returns a mutable pointer to the chunk with the given ID WITHOUT CHECKING BOUNDS.
    //! If the chunk is already loaded, it will remain loaded until the next call
    //! to `prune()`. Otherwise, `nullptr` will be returned.
    Chunk* getChunkAtIdUnchecked(ChunkId aChunkId) {
        auto& chunk =
            _chunks[static_cast<hg::PZInteger>(aChunkId.y)][static_cast<hg::PZInteger>(aChunkId.x)];
        if (HG_LIKELY_CONDITION(!chunk.isEmpty())) {
            HG_LIKELY_BRANCH;
            return &chunk;
        } else {
            HG_UNLIKELY_BRANCH;
            return nullptr;
        }
    }

    //! Returns a const pointer to the chunk with the given ID WITHOUT CHECKING BOUNDS.
    //! If the chunk is already loaded, it will remain loaded until the next call
    //! to `prune()`. Otherwise, `nullptr` will be returned.
    const Chunk* getChunkAtIdUnchecked(ChunkId aChunkId) const {
        return const_cast<Self*>(this)->getChunkAtIdUnchecked(aChunkId);
    }

    ///////////////////////////////////////////////////////////////////////////
    // CELL GETTERS                                                          //
    ///////////////////////////////////////////////////////////////////////////

    //! Returns a mutable reference to the cell at (aX, aY) WITHOUT CHECKING BOUNDS.
    //! The containing chunk will be loaded if it isn't already, and will remain loaded until
    //! the next call to `prune()`.
    CellModelExt& getCellAtUnchecked(hg::PZInteger aX, hg::PZInteger aY, LOAD_IF_MISSING_Tag) {
        const auto chunkX = aX / _chunkWidth;
        const auto chunkY = aY / _chunkHeight;

        auto& chunk = _chunks[chunkY][chunkX];
        if (HG_LIKELY_CONDITION(!chunk.isEmpty())) {
            HG_LIKELY_BRANCH;
            return chunk._getCellExtAtUnchecked(aX % _chunkWidth, aY % _chunkHeight);
        } else {
            HG_UNLIKELY_BRANCH;
            _loadChunkImmediately({chunkX, chunkY});
            return chunk._getCellExtAtUnchecked(aX % _chunkWidth, aY % _chunkHeight);
        }
    }

    //! Returns a const reference to the cell at (aX, aY) WITHOUT CHECKING BOUNDS.
    //! The containing chunk will be loaded if it isn't already, and will remain loaded until
    //! the next call to `prune()`.
    const CellModelExt& getCellAtUnchecked(hg::PZInteger aX,
                                           hg::PZInteger aY,
                                           LOAD_IF_MISSING_Tag) const {
        return const_cast<Self*>(this)->getCellAtUnchecked(aX, aY, LOAD_IF_MISSING);
    }

    //! Returns a mutable pointer to the cell at (aX, aY) WITHOUT CHECKING BOUNDS.
    //! If the containing chunk is already loaded, it will remain loaded until the next call
    //! to `prune()`. Otherwise, `nullptr` will be returned.
    CellModelExt* getCellAtUnchecked(hg::PZInteger aX, hg::PZInteger aY) {
        const auto chunkX = aX / _chunkWidth;
        const auto chunkY = aY / _chunkHeight;

        auto& chunk = _chunks[chunkY][chunkX];
        if (HG_LIKELY_CONDITION(!chunk.isEmpty())) {
            HG_LIKELY_BRANCH;
            return &(chunk._getCellExtAtUnchecked(aX % _chunkWidth, aY % _chunkHeight));
        } else {
            HG_UNLIKELY_BRANCH;
            return nullptr;
        }
    }

    //! Returns a const pointer to the cell at (aX, aY) WITHOUT CHECKING BOUNDS.
    //! If the containing chunk is already loaded, it will remain loaded until the next call
    //! to `prune()`. Otherwise, `nullptr` will be returned.
    const CellModelExt* getCellAtUnchecked(hg::PZInteger aX, hg::PZInteger aY) const {
        return const_cast<Self*>(this)->getCellAtUnchecked(aX, aY);
    }

private:
    friend class ActiveArea;
    
    // ===== Binder

    Binder* _binder = nullptr;

    // ===== Chunk Grid

    mutable hg::util::RowMajorGrid<Chunk> _chunks;

    // ===== Active Chunks

    struct ChunkControlBlock {
        std::shared_ptr<ChunkSpoolerInterface::RequestHandleInterface> requestHandle = nullptr;
        hg::PZInteger                                                  usageCount    = 0;
    };
    
    mutable std::unordered_map<ChunkId, ChunkControlBlock> _chunkControlBlocks;

    // ===== Free Chunks

    using Timestamp = std::chrono::steady_clock::time_point;

    //! This collection maps ChunkId -> Timestamp of the chunk becoming free.
    //! It is by default sorted in ascending order, meaning that the lowest (least recent)
    //! timestamps will be in the front, thus the chunks in the front are the best ones to evict.
    mutable hg::util::ValueSortedMap<ChunkId, Timestamp> _freeChunks;

    // ===== Members

    ChunkSpoolerInterface* _chunkSpooler = nullptr;

    hg::PZInteger _chunkWidth;
    hg::PZInteger _chunkHeight;

    hg::PZInteger _freeChunkLimit;

    // ===== Methods

    void _loadChunkImmediately(ChunkId aChunkId);

    void _onChunkLoaded(
        ChunkId                                                aChunkId,
        Chunk&&                                                aChunk,
        std::optional<decltype(_chunkControlBlocks)::iterator> aControlBlockIterator);

    void _createDefaultChunk(ChunkId aChunkId);

    void _updateChunkUsage(const std::vector<detail::ChunkUsageChange>& aChunkUsageChanges);

};

} // namespace detail

} // namespace gridworld
