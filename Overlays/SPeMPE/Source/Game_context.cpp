// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <SPeMPE/GameContext/Game_context.hpp>

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>

namespace jbatnozic {
namespace spempe {

namespace {
constexpr const char* LOG_ID = "SPeMPE";
} // namespace

GameContext::GameContext(const RuntimeConfig& aRuntimeConfig, hg::PZInteger aComponentTableSize)
    : _runtimeConfig{aRuntimeConfig}
    , _qaoRuntime{this}
    , _components{aComponentTableSize} {
    HG_VALIDATE_ARGUMENT(_runtimeConfig.tickRate.getValue() > 0);
    HG_VALIDATE_ARGUMENT(_runtimeConfig.maxConsecutiveUpdates > 0);
}

GameContext::~GameContext() {
    cleanUp();
}

void GameContext::cleanUp() {
    _qaoRuntime.destroyAllOwnedObjects();
    //_postStepActions.clear(); TODO

    while (!_ownedComponents.empty()) {
        ContextComponent& component = *_ownedComponents.back();
        _components.detachComponent(component);
        _ownedComponents.pop_back();
    }
}

///////////////////////////////////////////////////////////////////////////
// MARK: CONFIGURATION                                                   //
///////////////////////////////////////////////////////////////////////////

void GameContext::setToMode(Mode aMode) {
    // TODO
    _mode = aMode;
}

bool GameContext::isPrivileged() const {
    return ((static_cast<int>(_mode) & detail::GCMF_PRIV) != 0);
}

bool GameContext::isHeadless() const {
    return ((static_cast<int>(_mode) & detail::GCMF_HDLS) != 0);
}

bool GameContext::hasNetworking() const {
    return ((static_cast<int>(_mode) & detail::GCMF_NETW) != 0);
}

const GameContext::RuntimeConfig& GameContext::getRuntimeConfig() const {
    return _runtimeConfig;
}

///////////////////////////////////////////////////////////////////////////
// MARK: GAME STATE MANAGEMENT                                           //
///////////////////////////////////////////////////////////////////////////

GameContext::GameState& GameContext::getGameState() {
    return _state;
}

const GameContext::GameState& GameContext::getGameState() const {
    return _state;
}

///////////////////////////////////////////////////////////////////////////
// MARK: GAME OBJECT MANAGEMENT                                          //
///////////////////////////////////////////////////////////////////////////

hg::QAO_Runtime& GameContext::getQAORuntime() {
    return _qaoRuntime;
}

///////////////////////////////////////////////////////////////////////////
// MARK: COMPONENT MANAGEMENT                                            //
///////////////////////////////////////////////////////////////////////////

std::string GameContext::getComponentTableString(char aSeparator) const {
    return _components.toString(aSeparator);
}

///////////////////////////////////////////////////////////////////////////
// MARK: EXECUTION                                                       //
///////////////////////////////////////////////////////////////////////////

int GameContext::runFor(int aIterations) {
    int rv;
    _quit.store(false);
    _runImpl(this, &rv, aIterations);
    return rv;
}

void GameContext::stop() {
    if (hasChildContext() && isChildContextJoinable()) {
        const auto childRv = stopAndJoinChildContext();
        HG_LOG_INFO(LOG_ID, "Child context stopped with exit code {}.", childRv);
    }

    _quit.store(true);
}

const GameContext::PerformanceInfo& GameContext::getPerformanceInfo() const {
    return _performanceInfo;
}

std::uint64_t GameContext::getCurrentIterationOrdinal() const {
    return _iterationCounter;
}

///////////////////////////////////////////////////////////////////////////
// MARK: CHILD CONTEXT SUPPORT                                           //
///////////////////////////////////////////////////////////////////////////

void GameContext::attachChildContext(std::unique_ptr<GameContext> aChildContext) {
    if (hasChildContext()) {
        HG_THROW_TRACED(hg::TracedLogicError, 0, "A child context is already attached.");
    }
    _childContext                 = std::move(aChildContext);
    _childContext->_parentContext = this;
}

std::unique_ptr<GameContext> GameContext::detachChildContext() {
    if (hasChildContext() && isChildContextJoinable()) {
        HG_THROW_TRACED(hg::TracedLogicError,
                        0,
                        "Cannot detach a running child context - stop and join it first.");
    }

    return std::move(_childContext);
}

bool GameContext::hasChildContext() const {
    return (_childContext != nullptr);
}

bool GameContext::isChildContextJoinable() const {
    if (!hasChildContext()) {
        HG_THROW_TRACED(hg::TracedLogicError, 0, "No child context is currently attached.");
    }
    return _childContextThread.joinable();
}

GameContext* GameContext::getChildContext() const {
    return _childContext.get();
}

void GameContext::startChildContext(int aIterations) {
    if (!hasChildContext()) {
        HG_THROW_TRACED(hg::TracedLogicError, 0, "No child context is currently attached");
    }

    if (_childContextThread.joinable()) {
        HG_THROW_TRACED(hg::TracedLogicError,
                        0,
                        "The previous child context must be stopped and joined first.");
    }

    _childContext->_quit.store(false);
    _childContextThread =
        std::thread{_runImpl, _childContext.get(), &_childContextReturnValue, aIterations, false};
}

int GameContext::stopAndJoinChildContext() {
    if (!hasChildContext()) {
        return _childContextReturnValue;
    }

    _childContext->stop();

    if (_childContextThread.joinable()) {
        _childContextThread.join();
    }

    return _childContextReturnValue;
}

///////////////////////////////////////////////////////////////////////////
// MARK: PRIVATE METHODS                                                 //
///////////////////////////////////////////////////////////////////////////

namespace {

#if HG_BUILD_TYPE == HG_DEBUG
// If we're in Debug mode, we don't want to catch and handle
// exceptions, but rather let the IDE break the program.
#define CATCH_EXCEPTIONS_TOP_LEVEL
#endif

int DoSingleQaoIteration(hg::QAO_Runtime& runtime, std::int32_t eventFlags) {
    runtime.startStep();
    bool done = false;
    do {
#ifdef CATCH_EXCEPTIONS_TOP_LEVEL
        try {
            runtime.advanceStep(done, eventFlags);
        } catch (std::exception& ex) {
            HG_LOG_ERROR(LOG_ID, "Exception caught: {}", ex.what());
            return 1;
        } catch (...) {
            HG_LOG_ERROR(LOG_ID, "Unknown exception caught!");
            return 2;
        }
#else
        runtime.advanceStep(done, eventFlags);
#endif
    } while (!done);

    return 0;
}

using hg::QAO_ALL_EVENT_FLAGS;
using hg::QAO_Event;

constexpr std::int32_t ToEventMask(QAO_Event::Enum ev) {
    return (1 << static_cast<std::int32_t>(ev));
}

// clang-format off
constexpr std::int32_t QAO_EVENT_MASK_ALL_DRAWS = ToEventMask(QAO_Event::PRE_DRAW)
                                                | ToEventMask(QAO_Event::DRAW_1)
                                                | ToEventMask(QAO_Event::DRAW_2)
                                                | ToEventMask(QAO_Event::DRAW_GUI)
                                                | ToEventMask(QAO_Event::POST_DRAW);

constexpr std::int32_t QAO_EVENT_MASK_DISPLAY = ToEventMask(QAO_Event::DISPLAY);

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_DRAW = QAO_ALL_EVENT_FLAGS & ~QAO_EVENT_MASK_ALL_DRAWS;

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_DISPLAY = QAO_ALL_EVENT_FLAGS & ~QAO_EVENT_MASK_DISPLAY;

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_DRAW_AND_DISPLAY = QAO_EVENT_MASK_ALL_EXCEPT_DRAW
                                                                  & QAO_EVENT_MASK_ALL_EXCEPT_DISPLAY;
// clang-format on

using TimingDuration = std::chrono::duration<double, std::micro>;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

template <class taDuration>
double MsCount(taDuration aDuration) {
    return static_cast<double>(duration_cast<microseconds>(aDuration).count() / 1000.0);
}
} // namespace

void GameContext::_runImpl(hg::NeverNull<GameContext*> aContext,
                           hg::NeverNull<int*>         aReturnValue,
                           int                         aMaxIterations,
                           bool                        aDebugLoggingActive) {
    using hobgoblin::util::Stopwatch;
#define DebugLog(...)                          \
    do {                                       \
        if (aDebugLoggingActive)               \
            HG_LOG_DEBUG(LOG_ID, __VA_ARGS__); \
    } while (false)

    if (aMaxIterations == 0) {
        (*aReturnValue) = 0;
        return;
    }

    const auto           maxConsecutiveUpdates = aContext->_runtimeConfig.maxConsecutiveUpdates;
    const TimingDuration deltaTime             = aContext->_runtimeConfig.tickRate.getDeltaTime();

    DebugLog("_runImpl - CONFIG - maxConsecutiveUpdates={}, deltaTime={}ms.",
             maxConsecutiveUpdates,
             MsCount(deltaTime));

    aContext->_performanceInfo.updateStart  = std::chrono::steady_clock::now();
    aContext->_performanceInfo.drawStart    = std::chrono::steady_clock::now();
    aContext->_performanceInfo.displayStart = std::chrono::steady_clock::now();

    int            iterationsCovered = 0;
    TimingDuration accumulator       = deltaTime;
    Stopwatch      frameToFrameStopwatch;

    while (true) {
        DebugLog("_runImpl - ITERATION start");

        if (aMaxIterations > 0 && iterationsCovered >= aMaxIterations) {
            DebugLog("_runImpl - Outer while loop breaking because aMaxIterations was reached.");
            break;
        }
        if (aContext->_quit.load()) {
            DebugLog("_runImpl - Outer while loop breaking because _quit was set to true.");
            break;
        }

        aContext->_performanceInfo.consecutiveUpdateSteps = 0;
        for (int i = 0; i < maxConsecutiveUpdates; i += 1) {
            if (aMaxIterations > 0 && iterationsCovered >= aMaxIterations) {
                DebugLog("_runImpl - Inner for loop breaking because aMaxIterations was reached.");
                break;
            }
            if (accumulator < deltaTime) {
                DebugLog("_runImpl - Inner for loop breaking because accumulator is too low ({}ms).",
                         MsCount(accumulator));
                break;
            }
            if (aContext->_quit.load()) {
                DebugLog("_runImpl - Inner for loop breaking because _quit was set to true.");
                break;
            }

            aContext->_performanceInfo.consecutiveUpdateSteps += 1;

            // UPDATE
            {
                Stopwatch updateStopwatch;
                aContext->_performanceInfo.updateStart = std::chrono::steady_clock::now();
                DebugLog("_runImpl - UPDATE start");
                (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime,
                                                       QAO_EVENT_MASK_ALL_EXCEPT_DRAW_AND_DISPLAY);
                DebugLog("_runImpl - UPDATE end (status = {})", *aReturnValue);
                if ((*aReturnValue) != 0) {
                    return;
                }
                aContext->_performanceInfo.updateTime = updateStopwatch.getElapsedTime<nanoseconds>();
            }

            accumulator -= deltaTime;

            iterationsCovered += 1;
            aContext->_iterationCounter += 1;

            // TODO(poll post step actions)
        } // End for

        const bool didAtLeastOneUpdate = (aContext->_performanceInfo.consecutiveUpdateSteps > 0);

        if (didAtLeastOneUpdate && !aContext->isHeadless()) {
            // DRAW
            {
                Stopwatch drawStopwatch;
                aContext->_performanceInfo.drawStart = std::chrono::steady_clock::now();
                DebugLog("_runImpl - DRAW start");
                (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime, QAO_EVENT_MASK_ALL_DRAWS);
                DebugLog("_runImpl - DRAW end (status = {})", *aReturnValue);
                if ((*aReturnValue) != 0) {
                    return;
                }
                aContext->_performanceInfo.drawTime = drawStopwatch.getElapsedTime<nanoseconds>();
            }
        }

        if (didAtLeastOneUpdate) {
            // Prevent excessive buildup in accumulator in case
            // the program is not meeting time requirements
            const auto accBefore = accumulator;
            accumulator          = std::min(accumulator, deltaTime * 0.5);
            if (accumulator != accBefore) {
                DebugLog("_runImpl - Accumulator clamped from {}ms to {}ms.",
                         MsCount(accBefore),
                         MsCount(accumulator));
            }
        }

        // DISPLAY
        {
            Stopwatch displayStopwatch;
            aContext->_performanceInfo.displayStart = std::chrono::steady_clock::now();
            DebugLog("_runImpl - DISPLAY start");
            (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime, QAO_EVENT_MASK_DISPLAY);
            DebugLog("_runImpl - DISPLAY end (status = {})", *aReturnValue);
            if ((*aReturnValue) != 0) {
                return;
            }
            aContext->_performanceInfo.displayTime = displayStopwatch.getElapsedTime<nanoseconds>();
        }

        // 'Refill' accumulator
        const auto elapsedTime = frameToFrameStopwatch.restart<TimingDuration>();
        accumulator += elapsedTime;
        DebugLog("_runImpl - Accumulator increased by {}ms (new value: {}ms).",
                 MsCount(elapsedTime),
                 MsCount(accumulator));

        DebugLog("_runImpl - ITERATION end");
    } // End while

    (*aReturnValue) = 0;
#undef DebugLog
}

} // namespace spempe
} // namespace jbatnozic
