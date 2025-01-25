// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#ifndef SPEMPE_GAME_CONTEXT_GAME_CONTEXT_HPP
#define SPEMPE_GAME_CONTEXT_GAME_CONTEXT_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/QAO.hpp>

#include <SPeMPE/GameContext/Context_components.hpp>
#include <SPeMPE/GameObjectFramework/Synchronized_object_registry.hpp>
#include <SPeMPE/Utility/Timing.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace jbatnozic {
namespace spempe {

namespace hg = ::jbatnozic::hobgoblin;

namespace detail {
// Game Context Mode Flags
constexpr int GCMF_NONE = 0x0;
constexpr int GCMF_PRIV = 0x1;
constexpr int GCMF_HDLS = 0x2;
constexpr int GCMF_NETW = 0x4;

// Game Context Modes
constexpr int GCM_INIT = GCMF_NONE;
constexpr int GCM_SERV = GCMF_PRIV | GCMF_HDLS | GCMF_NETW;
constexpr int GCM_CLNT = GCMF_NETW;
constexpr int GCM_SOLO = GCMF_PRIV;
constexpr int GCM_GMAS = GCMF_PRIV | GCMF_NETW;
} // namespace detail

//! TODO: Components included by default: WindowManager, LoggingManager, (NetworkingManager?)
class GameContext {
public:
    ///////////////////////////////////////////////////////////////////////////
    // MARK: CONFIGURATION                                                   //
    ///////////////////////////////////////////////////////////////////////////

    // clang-format off
    enum class Mode {
                                       // PRIVILEGED | HEADLESS | NETWORKING
        Initial    = detail::GCM_INIT, //            |          |           
        Server     = detail::GCM_SERV, //      +     |     +    |      +    
        Client     = detail::GCM_CLNT, //            |          |      +    
        Solo       = detail::GCM_SOLO, //      +     |          |           
        GameMaster = detail::GCM_GMAS, //      +     |          |      +    
    };
    // clang-format on

    struct RuntimeConfig {
        //! The desired number of ticks per second for your game. 'Ticks' are
        //! logical steps in the game, where game world simulation happens, separated
        //! from the number of literal frames the game produces per second. One
        //! tick corresponds to one QAO Update step, usually (but not necessarily)
        //! followed by a QAO Draw step.
        TickRate tickRate = TickRate{60};

        //! The maximum number of frames that can be simulated between two
        //! render/draw steps, in case the application isn't achieving the
        //! desired framerate. The recommended value is 2.
        hg::PZInteger maxConsecutiveUpdates{2};
    };

    GameContext(const RuntimeConfig& aRuntimeConfig, hg::PZInteger aComponentTableSize = 32);
    ~GameContext();

    void cleanUp();

    //! Only transitions between Initial and other modes (and the other way
    //! round) are possible.
    void setToMode(Mode aMode);

    bool isPrivileged() const;
    bool isHeadless() const;
    bool hasNetworking() const;

    const RuntimeConfig& getRuntimeConfig() const;

    ///////////////////////////////////////////////////////////////////////////
    // MARK: GAME STATE MANAGEMENT                                           //
    ///////////////////////////////////////////////////////////////////////////

    struct GameState {
        bool isPaused = false;
        // TODO: isStopping
    };

    GameState&       getGameState();
    const GameState& getGameState() const;

    ///////////////////////////////////////////////////////////////////////////
    // MARK: GAME OBJECT MANAGEMENT                                          //
    ///////////////////////////////////////////////////////////////////////////

    hg::QAO_Runtime& getQAORuntime();

    ///////////////////////////////////////////////////////////////////////////
    // MARK: COMPONENT MANAGEMENT                                            //
    ///////////////////////////////////////////////////////////////////////////

    //! Throws hg::TracedLogicError in case of tagHash clash.
    template <class taComponent>
    void attachComponent(taComponent& aComponent);

    //! Throws hg::TracedLogicError in case of tagHash clash.
    template <class taComponent>
    void attachAndOwnComponent(std::unique_ptr<taComponent> aComponent);

    //! Attempt to detach a context component from the context.
    //! One of three things can happen:
    //! - If component is found and owned by the context:
    //!   `aDetachStatus` set to `DetachStatus::OK`, unique pointer to component returned.
    //!
    //! - If component is found but not owned by the context:
    //!   `aDetachStatus` set to `DetachStatus::NOT_OWNED_BY_CONTEXT`, nullptr returned.
    //!
    //! - If component is not found:
    //!   `aDetachStatus` set to `DetachStatus::NOT_FOUND`, nullptr returned.
    template <class taComponent>
    std::unique_ptr<taComponent> detachComponent(hg::NeverNull<DetachStatus*> aDetachStatus);

    //! Throws hg::TracedLogicError if the component isn't present.
    template <class taComponent>
    taComponent& getComponent() const;

    //! Returns nullptr if the component isn't present.
    template <class taComponent>
    taComponent* getComponentPtr() const;

    std::string getComponentTableString(char aSeparator = '\n') const;

    ///////////////////////////////////////////////////////////////////////////
    // MARK: EXECUTION                                                       //
    ///////////////////////////////////////////////////////////////////////////

    //! Run for a number of iterations. A negative value will be taken as infinity.
    //! Returns status code of the run (0 = success).
    int runFor(int aIterations);

    //! Can be called by an instance from 'within' the context (if runFor has
    //! been started) to stop the execution after the current step.
    void stop();

    struct PerformanceInfo {
        using nanoseconds  = std::chrono::nanoseconds;
        using steady_clock = std::chrono::steady_clock;

        //! Time point showing the time when the most recent QAO Update step was
        //! started (undefined if no Update step was started yet).
        //! This time point is relative to the start of an undefined epoch, so
        //! only comparing to another `std::chrono::steady_clock::time_point`
        //! makes sense.
        steady_clock::time_point updateStart;

        //! Duration of the last finished Update step.
        nanoseconds updateTime{0};

        //! Time point showing the time when the most recent QAO Draw step was
        //! started (undefined if no Draw step was started yet).
        //! This time point is relative to the start of an undefined epoch, so
        //! only comparing to another `std::chrono::steady_clock::time_point`
        //! makes sense.
        steady_clock::time_point drawStart;

        //! Duration of the last finished Draw step.
        nanoseconds drawTime{0};

        //! Time point showing the time when the most recent QAO Display step was
        //! started (undefined if no Display step was started yet).
        //! This time point is relative to the start of an undefined epoch, so
        //! only comparing to another `std::chrono::steady_clock::time_point`
        //! makes sense.
        steady_clock::time_point displayStart;

        //! Duration of the last finished Display step.
        nanoseconds displayTime{0};

        //! Duration of the last fully finished iteration.
        nanoseconds iterationTime{0};

        //! Number of consecutive Update steps in an iteration.
        //! During the first Update step in an iteration, this variable
        //! will be set to 1, then during the next consecutive Update step
        //! (if it happens) it will be set to 2, and so on.
        hg::PZInteger consecutiveUpdateSteps{0};
    };

    //! Returns the performance measurements of the last executed step.
    const PerformanceInfo& getPerformanceInfo() const;

    //! Returns the ordinal number of the step currently being executed, or of
    //! the last step that was executed if execution was stopped or finished.
    std::uint64_t getCurrentIterationOrdinal() const;

    ///////////////////////////////////////////////////////////////////////////
    // MARK: CHILD CONTEXT SUPPORT                                           //
    ///////////////////////////////////////////////////////////////////////////

    //! Attach a child context.
    //! Throws hg::TracedLogicError if a context is already attached.
    void attachChildContext(std::unique_ptr<GameContext> aChildContext);

    //! Detaches the currently attached child context (return value can be
    //! nullptr if none).
    //! Throws hg::TracedLogicError if the attached context is currently running
    //! (that is, if isChildContextJoinable() returns true).
    std::unique_ptr<GameContext> detachChildContext();

    //! Returns true if a child context is currently attached.
    bool hasChildContext() const;

    //! Returns true if the attached child context is currently running or has
    //! finished execution and is waiting to be joined. Also returns true if
    //! it hasn't been started yet.
    //! Throws hg::TracedLogicError if no child context is attached.
    bool isChildContextJoinable() const;

    //! Returns the currently attached child context (can be nullptr if none).
    GameContext* getChildContext() const;

    //! Starts executing the child context in a background thread.
    //! The argument aIterations is the same as for runFor().
    //! Throws hg::TracedLogicError if no child context is attached or it's
    //! joinable (that is, isChildContextJoinable() returns true).
    void startChildContext(int aIterations);

    //! If a child context is currently attached and is currently running, this
    //! method stops it and joins the background thread. Otherwise, it does
    //! nothing. The return value is the status of the execution of the last
    //! stopped child context (0 = success), which will be undefined if no child
    //! contexts were ever started.
    int stopAndJoinChildContext();

private:
    // Configuration:
    Mode          _mode = Mode::Initial;
    RuntimeConfig _runtimeConfig;

    // State:
    GameState _state;

    // Game object management:
    hg::QAO_Runtime _qaoRuntime;

    // Context components:
    std::vector<std::unique_ptr<ContextComponent>> _ownedComponents;
    detail::ComponentTable                         _components;

    // Execution:
    PerformanceInfo   _performanceInfo;
    std::uint64_t     _iterationCounter = 0;
    std::atomic<bool> _quit;

    // Child context support:
    GameContext*                 _parentContext = nullptr;
    std::unique_ptr<GameContext> _childContext  = nullptr;
    std::thread                  _childContextThread;
    int                          _childContextReturnValue = 0;

    static void _runImpl(hg::NeverNull<GameContext*> aContext,
                         hg::NeverNull<int*>         aReturnValue,
                         int                         aMaxIterations,
                         bool                        aDebugLoggingActive = false);
};

template <class taComponent>
void GameContext::attachComponent(taComponent& aComponent) {
    _components.attachComponent(aComponent);
}

template <class taComponent>
void GameContext::attachAndOwnComponent(std::unique_ptr<taComponent> aComponent) {
    HG_VALIDATE_ARGUMENT(aComponent != nullptr, "Cannot attach null component.");

    _components.attachComponent(*aComponent);
    _ownedComponents.push_back(std::move(aComponent));
}

template <class taComponent>
std::unique_ptr<taComponent> GameContext::detachComponent(hg::NeverNull<DetachStatus*> aDetachStatus) {
    auto* ptr = getComponentPtr<taComponent>();
    if (ptr == nullptr) {
        *aDetachStatus = DetachStatus::NOT_FOUND;
        return nullptr;
    }

    const auto iter = std::find_if(_ownedComponents.begin(),
                                   _ownedComponents.end(),
                                   [ptr](std::unique_ptr<ContextComponent>& aCurr) {
                                       return aCurr.get() == ptr;
                                   });
    if (iter == _ownedComponents.end()) {
        _components.detachComponent(*ptr);
        *aDetachStatus = DetachStatus::NOT_OWNED_BY_CONTEXT;
        return nullptr;
    }

    std::unique_ptr<ContextComponent> temp = std::move(*iter);
    std::unique_ptr<taComponent>      result =
        std::unique_ptr<taComponent>{static_cast<taComponent*>(temp.release())};
    _components.detachComponent(*result);
    _ownedComponents.erase(iter);
    *aDetachStatus = DetachStatus::OK;
    return result;
}

template <class taComponent>
taComponent& GameContext::getComponent() const {
    return _components.getComponent<taComponent>();
}

template <class taComponent>
taComponent* GameContext::getComponentPtr() const {
    return _components.getComponentPtr<taComponent>();
}

} // namespace spempe
} // namespace jbatnozic

// Convenience #include
#include <SPeMPE/GameContext/Game_context_flag_validation.hpp>

#endif // !SPEMPE_GAME_CONTEXT_GAME_CONTEXT_HPP
