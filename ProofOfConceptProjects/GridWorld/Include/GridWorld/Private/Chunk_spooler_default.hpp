#pragma once

#include "Chunk_disk_io_handler_interface.hpp"

#include <GridWorld/Private/Chunk_spooler_interface.hpp>

#include <Hobgoblin/Common.hpp>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <variant>

namespace gridworld {
namespace detail {

namespace hg = ::jbatnozic::hobgoblin;

class RequestHandleImpl;

class DefaultChunkSpooler final : public ChunkSpoolerInterface {
public:
    DefaultChunkSpooler();
    ~DefaultChunkSpooler() override;

    void setDiskIoHandler(ChunkDiskIoHandlerInterface* aDiskIoHandler) override;

    void pause() override;
    void unpause() override;
    bool isPaused() const override;

    std::vector<std::shared_ptr<RequestHandleInterface>> loadChunks(
        const std::vector<LoadRequest>& aLoadRequests) override;

    hg::PZInteger unloadChunk(ChunkId aChunkId, Chunk&& aChunk) override;

    void unloadRuntimeCache() override;

private:
    friend class RequestHandleImpl;

    ChunkDiskIoHandlerInterface* _diskIoHandler;

    using Mutex = std::mutex;
    mutable Mutex _mutex;

    bool _paused  = false;
    bool _stopped = false;

    std::condition_variable _cv_workerSync;

    struct UnloadRequest {
        Chunk   chunk;
        ChunkId id;
    };

    static constexpr int UNLOAD_REQUEST_DEFAULT_PRIORITY = 10;
    static constexpr int UNLOAD_REQUEST_THRESHOLD        = 32;
    static constexpr int LOADS_PER_UNLOAD                = 2;

    using RequestVariant = std::variant<std::monostate, LoadRequest, UnloadRequest>;

    struct RequestControlBlock {
        RequestVariant                     request;
        std::shared_ptr<RequestHandleImpl> handle;
    };

    std::unordered_map<ChunkId, RequestControlBlock> _requests;
    using RequestIter = decltype(_requests)::iterator;

    hg::PZInteger _loadRequestCount   = 0;
    hg::PZInteger _unloadRequestCount = 0;
    int           _unloadPriority     = 0;

    std::thread _worker;

    void _workerBody();

    std::shared_ptr<RequestHandleInterface> _onNewLoadRequest(LoadRequest aLoadRequest,
                                                              const std::unique_lock<Mutex>&);
    hg::PZInteger       _countAvailableRequests(const std::unique_lock<Mutex>&) const;
    RequestIter         _findLoadRequestWithBestPriority(const std::unique_lock<Mutex>&);
    RequestIter         _findAnyUnloadRequest(const std::unique_lock<Mutex>&);
    RequestIter         _findRequestWithBestPriority(const std::unique_lock<Mutex>&);
    RequestControlBlock _eraseRequest(RequestIter aRequestIter, const std::unique_lock<Mutex>&);

    void                         _cancelLoadRequest(ChunkId aChunkId);
    std::optional<hg::PZInteger> _swapLoadRequestPriority(ChunkId aChunkId, hg::PZInteger aNewPriority);
    std::optional<hg::PZInteger> _boostLoadRequestPriority(ChunkId aChunkId, hg::PZInteger aNewPriority);

    void _adjustUnloadPriority(const RequestVariant& aRequestVariant, const std::unique_lock<Mutex>&);
};

} // namespace detail
} // namespace gridworld
