#pragma once

#include <GridWorld/Private/Chunk_disk_io_handler_interface.hpp>

#include <GridWorld/Model/Chunk.hpp>
#include <GridWorld/Private/Model_conversions.hpp>

#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace gridworld {
namespace test {

class FakeDiskIoHandler : public detail::ChunkDiskIoHandlerInterface{
public:
    void setBinder(Binder*) override {}

    std::optional<Chunk> loadChunkFromRuntimeCache(ChunkId aChunkId) override {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});

        const auto iter = _runtimeCache.find(aChunkId);
        if (iter == _runtimeCache.end()) {
            return {};
        }
        return detail::JsonStringToChunk(iter->second);
    }

    void storeChunkInRuntimeCache(const Chunk& aChunk, ChunkId aChunkId) override {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        _runtimeCache[aChunkId] = detail::ChunkToJsonString(aChunk);
    }

    std::optional<Chunk> loadChunkFromPersistentCache(ChunkId aChunkId) override {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});

        const auto iter = _persistentCache.find(aChunkId);
        if (iter == _persistentCache.end()) {
            return {};
        }
        return detail::JsonStringToChunk(iter->second);
    }

    void storeChunkInPersistentCache(const Chunk& aChunk, ChunkId aChunkId) override {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
        _persistentCache[aChunkId] = detail::ChunkToJsonString(aChunk);
    }

private:
    std::unordered_map<ChunkId, std::string> _runtimeCache;
    std::unordered_map<ChunkId, std::string> _persistentCache;
};

} // namespace test
} // namespace gridworld
