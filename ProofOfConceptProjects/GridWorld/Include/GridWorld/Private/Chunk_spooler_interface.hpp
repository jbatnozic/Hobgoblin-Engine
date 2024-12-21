#pragma once

#include <GridWorld/Model/Chunk.hpp>
#include <GridWorld/Model/Chunk_id.hpp>

#include <optional>
#include <vector>

namespace gridworld {
namespace detail {

// Forward declare
class ChunkDiskIoHandlerInterface;

//! Interface for a class handling loading an unloading of chunks asynchronously.
class ChunkSpoolerInterface {
public:
    //! Virtual destructor.
    virtual ~ChunkSpoolerInterface() = default;

    virtual void setDiskIoHandler(ChunkDiskIoHandlerInterface* aDiskIoHandler) = 0;

    virtual void pause() = 0;

    virtual void unpause() = 0;

    //! Checks whether the spooler is currently paused (returns true if yes).
    virtual bool isPaused() const = 0;

    struct LoadRequest {
        ChunkId       chunkId;  //!< ID of the chunk to be loaded.
        hg::PZInteger priority; //!< Lower number = higher priority
    };

    class RequestHandleInterface {
    public:
        //! Virtual destructor.
        //! \note Destroying the instance does NOT cancel the request automatically.
        virtual ~RequestHandleInterface() = default;

        //! Try to swap the priority of the request referred to by the handle with a new priortiy
        //! value. This can fail if the request is already being processed or has been cancelled or
        //! fully finished, in which cases the priority doesn't have a meaning anymore.
        //! 
        //! \returns previous priority if successful, or `std::nullopt` otherwise.
        virtual std::optional<hg::PZInteger> trySwapPriority(hg::PZInteger aNewPriority) = 0;

        //! Same as `trySwapPriority()`, but will only swap priorities if the new one is better.
        //! 
        //! \returns previous priority if successful, or `std::nullopt` otherwise.
        virtual std::optional<hg::PZInteger> tryBoostPriority(hg::PZInteger aNewPriority) = 0;

        //! Cancels the request. This call is idempotent.
        virtual void cancel() = 0;

        //! Returns the ID of the Chunk that was requested.
        virtual ChunkId getChunkId() const = 0;

        //! Returns `true` if the request has been processed and finished,
        //! and `false` otherwise.
        virtual bool isFinished() const = 0;

        //! Take the loaded chunk if the request was finished.
        //!
        //! \note the return value can be std::nullopt if the chunk wasn't found in
        //!       either cache.
        //!
        //! \throws hg::TracedLogicError if the request wasn't yet finished or the
        //!                              chunk was already taken.
        virtual std::optional<Chunk> takeChunk() = 0;
    };

    //! Requests from the spooler to load a batch of chunks.
    //!
    //! \returns a vector of request handles. Handle [N] will correspond to request [N] in the
    //!          input vector. These handles can be used to probe the status of each request and
    //!          to collect the results.
    //!
    //! \throws hg::TracedLogicError if there is a different request for any of the listed tiles
    //!                              already ongoing.
    virtual std::vector<std::shared_ptr<RequestHandleInterface>> loadChunks(
        const std::vector<LoadRequest>& aLoadRequests) = 0;

    //! TODO(description)
    //!
    //! The spooler must be paused when this is called.
    //! \throws hg::PreconditionNotMetError if the spooler is not paused when this method is called.
    virtual hg::PZInteger unloadChunk(ChunkId aChunkId, Chunk&& aChunk) = 0;

    //! TODO(description)
    //!
    //! The spooler must be paused when this is called.
    //! \throws hg::PreconditionNotMetError if the spooler is not paused when this method is called.
    virtual void unloadRuntimeCache() = 0;
};

} // namespace detail
} // namespace gridworld
