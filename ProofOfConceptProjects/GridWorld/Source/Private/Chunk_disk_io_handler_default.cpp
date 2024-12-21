#pragma once

#include <GridWorld/Private/Chunk_disk_io_handler_default.hpp>
#include <GridWorld/World/Binder.hpp>

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Utility/File_io.hpp>

#include <GridWorld/Private/Model_conversions.hpp>

#include <fstream>

namespace gridworld {
namespace detail {

namespace {
const std::filesystem::path CHUNKS_FOLDER = "DCIO_CHUNKS";
} // namespace

DefaultChunkDiskIoHandler::DefaultChunkDiskIoHandler(const WorldConfig& aConfig)
    : _basePath{aConfig.chunkDirectoryPath} {
    if (const auto path = _basePath / CHUNKS_FOLDER; !std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    }
}

void DefaultChunkDiskIoHandler::setBinder(Binder* aBinder) {
    _binder = aBinder;
}

DefaultChunkDiskIoHandler::~DefaultChunkDiskIoHandler() = default;

std::optional<Chunk> DefaultChunkDiskIoHandler::loadChunkFromRuntimeCache(ChunkId aChunkId) {
    return loadChunkFromPersistentCache(aChunkId);
}

void DefaultChunkDiskIoHandler::storeChunkInRuntimeCache(const Chunk& aChunk, ChunkId aChunkId) {
    storeChunkInPersistentCache(aChunk, aChunkId);
}

std::optional<Chunk> DefaultChunkDiskIoHandler::loadChunkFromPersistentCache(ChunkId aChunkId) {
    const auto path = _buildPathToChunk(aChunkId);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    auto bytes = hg::util::SlurpFileBytes(path);

    auto chunkExtensionFactory = [this]() {
        HG_ASSERT(_binder != nullptr);
        return _binder->createChunkExtension();
    };

    return JsonStringToChunk(std::move(bytes), chunkExtensionFactory);
}

void DefaultChunkDiskIoHandler::storeChunkInPersistentCache(const Chunk& aChunk, ChunkId aChunkId) {
    const auto path = _buildPathToChunk(aChunkId);

    auto str = ChunkToJsonString(aChunk);

    std::ofstream file{path, std::ios::out | std::ios::binary | std::ios::trunc};
    HG_HARD_ASSERT(file.is_open() && file.good());
    file << str;
}

std::filesystem::path DefaultChunkDiskIoHandler::_buildPathToChunk(ChunkId aChunkId) const {
    return _basePath / CHUNKS_FOLDER / fmt::format(FMT_STRING("chunk_{}_{}"), aChunkId.x, aChunkId.y);
}

} // namespace detail
} // namespace gridworld
