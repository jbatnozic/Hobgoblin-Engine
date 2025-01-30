// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

// clang-format off

#ifndef SPEMPE_GAME_OBJECT_FRAMEWORK_GAME_OBJECT_BASES_HPP
#define SPEMPE_GAME_OBJECT_FRAMEWORK_GAME_OBJECT_BASES_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/QAO.hpp>
#include <Hobgoblin/Utility/Dynamic_bitset.hpp>
#include <Hobgoblin/Utility/State_scheduler_simple.hpp>
#include <SPeMPE/GameContext/Game_context.hpp>
#include <SPeMPE/GameObjectFramework/Autodiff_state.hpp>
#include <SPeMPE/GameObjectFramework/Synchronized_object_registry.hpp>
#include <SPeMPE/GameObjectFramework/Sync_id.hpp>

#include <cassert>
#include <optional>
#include <typeinfo>
#include <type_traits>
#include <utility>

#define SPEMPE_TYPEID_SELF (typeid(decltype(*this)))

namespace jbatnozic {
namespace spempe {

namespace hg = ::jbatnozic::hobgoblin;

//! Not to be inherited from directly. Use one of the classes below.
class GameObjectBase : public hg::QAO_Base {
public:
    using QAO_Base::QAO_Base;

    //! Shorthand to get the GameContext in which this object lives.
    GameContext& ctx() const {
        auto* ctxPointer = getRuntime()->getUserData<GameContext>();
        assert(ctxPointer != nullptr);
        return *ctxPointer;
    }

    //! Shorthand to get one of the context components of the context 
    //! in which this object lives.
    template <class taComponent>
    taComponent& ccomp() const {
        return ctx().getComponent<taComponent>();
    }
};

///////////////////////////////////////////////////////////////////////////
// NONSTATE OBJECTS                                                      //
///////////////////////////////////////////////////////////////////////////

//! I:
//! Objects which are not essential to the game's state and thus not saved (when
//! writing game state) nor synchronized with clients (in multiplayer sessions).
//! For example, particle effects and such.
//! II:
//! Controllers which are always created when the game starts and thus always
//! implicitly present, so we don't need to save them.
class NonstateObject : public GameObjectBase {
public:
    using GameObjectBase::GameObjectBase;
};

///////////////////////////////////////////////////////////////////////////
// STATE OBJECTS                                                         //
///////////////////////////////////////////////////////////////////////////

//! Objects which are essential for the game's state, but will not be synchronized
//! with clients. For use with singleplayer games, or for server-side controller
//! objects in multiplayer games.
class StateObject : public GameObjectBase {
public:
    using GameObjectBase::GameObjectBase;
};

///////////////////////////////////////////////////////////////////////////
// SYNCHRONIZED OBJECTS                                                  //
///////////////////////////////////////////////////////////////////////////

enum class SyncUpdateStatus {
    Normal,    //! TODO (desc)
    Skip,      //! TODO (desc)
    Deactivate //! TODO (desc)
};

struct IfMaster {};
struct IfDummy  {};

class SynchronizedObjectBase : public StateObject {
public:
    //! Big scary constructor with way too many arguments.
    SynchronizedObjectBase(hg::QAO_RuntimeRef aRuntimeRef,
                           const std::type_info& aTypeInfo,
                           int aExecutionPriority,
                           std::string aName,
                           RegistryId aRegId,
                           SyncId aSyncId);

    ~SynchronizedObjectBase() override;

    SyncId getSyncId() const noexcept;
    bool isMasterObject() const noexcept;
    bool isUsingAlternatingUpdates() const noexcept;

    virtual bool isDeactivated() const = 0;

protected:
    // Call the following to sync this object's creation/update/destruction right away.

    void doSyncCreate() const;
    void doSyncUpdate() const;
    void doSyncDestroy() const;

    // These overloads will be called if the object is a Master object 
    // (that is, executing in a Privileged context).

    virtual void _eventPreUpdate(IfMaster)     {}
    virtual void _eventBeginUpdate(IfMaster)   {}
    virtual void _eventUpdate1(IfMaster)       {}
    virtual void _eventUpdate2(IfMaster)       {}
    virtual void _eventEndUpdate(IfMaster)     {}
    virtual void _eventPostUpdate(IfMaster)    {}
    virtual void _eventPreDraw(IfMaster)       {}
    virtual void _eventDraw1(IfMaster)         {}
    virtual void _eventDraw2(IfMaster)         {}
    virtual void _eventDrawGUI(IfMaster)       {}
    virtual void _eventPostDraw(IfMaster)      {}
    virtual void _eventDisplay(IfMaster)       {}

    // These overloads will be called if the object is a Dummy object 
    // (that is, executing in a non-Privileged context).

    virtual void _eventPreUpdate(IfDummy)     {}
    virtual void _eventBeginUpdate(IfDummy)   {}
    virtual void _eventUpdate1(IfDummy)       {}
    virtual void _eventUpdate2(IfDummy)       {}
    virtual void _eventEndUpdate(IfDummy)     {}
    virtual void _eventPostUpdate(IfDummy)    {}
    virtual void _eventPreDraw(IfDummy)       {}
    virtual void _eventDraw1(IfDummy)         {}
    virtual void _eventDraw2(IfDummy)         {}
    virtual void _eventDrawGUI(IfDummy)       {}
    virtual void _eventPostDraw(IfDummy)      {}
    virtual void _eventDisplay(IfDummy)       {}

    // If you override any of the below, the overloads above will not be used.
    // The same code will be executed on both ends.

    void _eventPreUpdate()   override;
    void _eventBeginUpdate() override;
    void _eventUpdate1()     override final;
    void _eventUpdate2()     override;
    void _eventEndUpdate()   override;
    void _eventPostUpdate()  override;
    void _eventPreDraw()     override;
    void _eventDraw1()       override;
    void _eventDraw2()       override;
    void _eventDrawGUI()     override;
    void _eventPostDraw()    override;
    void _eventDisplay()     override;

    // Misc.
    bool _willUpdateDeleteThis() const;

    void _enableAlternatingUpdates();

    //! This method can be called only during _eventFinalizeStep().
    //! It will return true if objects with alternating updates enabled have
    //! synced during this cycle.
    //! Usually this is only useful for objects that use both autodiff states
    //! and alternating updates, as they will want to commit their autodiff
    //! states only after a sync.
    bool _didAlternatingUpdatesSync() const;

private:
    detail::SynchronizedObjectRegistry& _syncObjReg;
    const SyncId _syncId;

    //! [0] = is it skipped for client 0?
    //! [1] = is it deactivated for client 0?
    //! [2] = is it skipped for client 1?
    //! [3] = is it deactivated for client 1?
    //! ...
    mutable hg::util::DynamicBitset _remoteSyncStatuses;

    int _deathCounter = -1;

    bool _alternatingUpdatesEnabled = false;

    //! Called when it's needed to sync this object's creation to one or more recepeints.
    virtual void _syncCreateImpl(SyncControlDelegate& aSyncCtrl) const = 0;

    //! Called when it's needed to sync this object's update to one or more recepeints.
    virtual void _syncUpdateImpl(SyncControlDelegate& aSyncCtrl) const = 0;

    //! Called when it's needed to sync this object's destruction to one or more recepeints.
    virtual void _syncDestroyImpl(SyncControlDelegate& aSyncCtrl) const = 0;

    virtual void _advanceDummyAndScheduleNewStates() = 0;

    virtual void _setStateSchedulerDefaultDelay(hg::PZInteger aNewDefaultDelaySteps) = 0;

public:
    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_destroySelfIn(int aStepCount);

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_syncCreateImpl(SyncControlDelegate& aSyncCtrl) const;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_syncUpdateImpl(SyncControlDelegate& aSyncCtrl) const;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_syncDestroyImpl(SyncControlDelegate& aSyncCtrl) const;

    //! \warning Internal implementation, do not call in user code!
    virtual void __spempeimpl_deactivateSelfIn(hg::PZInteger aDelaySteps) = 0;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_setDeactivationFlagForClient(hg::PZInteger aClientIdx, bool aFlag) const /*mutable*/;

    //! \warning Internal implementation, do not call in user code!
    bool __spempeimpl_getDeactivationFlagForClient(hg::PZInteger aClientIdx) const;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_setSkipFlagForClient(hg::PZInteger aClientIdx, bool aFlag) const /*mutable*/;

    //! \warning Internal implementation, do not call in user code!
    bool __spempeimpl_getSkipFlagForClient(hg::PZInteger aClientIdx) const;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_setNoDiffSkipFlagForClient(hg::PZInteger aClientIdx, bool aFlag) const /*mutable*/;

    //! \warning Internal implementation, do not call in user code!
    bool __spempeimpl_getNoDiffSkipFlagForClient(hg::PZInteger aClientIdx) const;

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_setStateSchedulerDefaultDelay(hg::PZInteger aNewDefaultDelaySteps);
};

//! Objects which are essential to the game's state, so they are both saved when
//! writing game state, and synchronized with clients in multiplayer sessions.
//! For example, units, terrain, interactible items (and, basically, most other 
//! game objects).
template <class taVisibleState>
class SynchronizedObject : public SynchronizedObjectBase {
public:
    using VisibleState = taVisibleState;

    bool isDeactivated() const final override {
        return _ssch.getCurrentState().status.isDeactivated;
    }

protected:
    using SyncObjSuper = SynchronizedObject;

    SynchronizedObject(hg::QAO_RuntimeRef aRuntimeRef,
                       const std::type_info& aTypeInfo,
                       int aExecutionPriority,
                       std::string aName,
                       RegistryId aRegId,
                       SyncId aSyncId = SYNC_ID_NEW)
        : SynchronizedObjectBase{ aRuntimeRef
                                , aTypeInfo
                                , aExecutionPriority
                                , std::move(aName)
                                , aRegId
                                , aSyncId
                                }
        , _ssch{
            isMasterObject() ? 0 // Masters don't need state scheduling
                             : reinterpret_cast<detail::SynchronizedObjectRegistry*>(
                                   aRegId.address
                               )->getDefaultDelay()
        }
    {
        _ssch.getCurrentState().status.isDeactivated = !isMasterObject();
    }

    taVisibleState& _getCurrentState() {
        assert(isMasterObject() || !isDeactivated());
        return _ssch.getCurrentState().visibleState;
    }

    taVisibleState& _getFollowingState() {
        assert(isMasterObject() || !isDeactivated());
        auto& followingState = _ssch.getFollowingState();
        if (followingState.status.isDeactivated) {
            return _ssch.getCurrentState().visibleState;
        }
        return followingState.visibleState;
    }

    const taVisibleState& _getCurrentState() const {
        assert(isMasterObject() || !isDeactivated());
        return _ssch.getCurrentState().visibleState;
    }

    const taVisibleState& _getFollowingState() const {
        return const_cast<SynchronizedObject*>(this)->_getFollowingState();
    }

//private:
protected:
    struct DummyStatus {
        bool isDeactivated = true;

        inline constexpr static DummyStatus active() {
            return DummyStatus{false};
        }

        inline constexpr static DummyStatus deactivated() {
            return DummyStatus{true};
        }
    };

    struct SchedulerPair {
        taVisibleState visibleState;
        DummyStatus status;

        friend
        std::ostream& operator<<(std::ostream& aOstream, const SchedulerPair& aSPair) {
            return (aOstream << aSPair.visibleState << (aSPair.status.isDeactivated ? " [D]" : ""));
        }
    };

    struct DeferredState {
        taVisibleState state;
        hg::PZInteger delay;
    };

    struct PacemakerPulse {
        bool happened = false;
        hg::PZInteger delay;
    };

    hg::util::SimpleStateScheduler<SchedulerPair> _ssch;

    std::optional<DeferredState> _deferredState; // Used with alternating updates

    PacemakerPulse _pacemakerPulse;

    void _advanceDummyAndScheduleNewStates() override final {
        _ssch.advance();

        if (_pacemakerPulse.happened) {
            _ssch.alignToDelay(_pacemakerPulse.delay);
            _pacemakerPulse.happened = false;
        }
        // Objects with alternating updates are more sensitive to lag, since they are receiving
        // half as many updates as other objects. Because of this, they can fall into a degenerate
        // state where their state scheduler is constantly out of blue states, and then they
        // receive a double update every other frame, which looks choppy and bad. This gets fixed
        // when a pacemaker pulse happens, but it can take a while before that happens. So, when
        // this degenerate state is detected, a pacemaker pulse is immediately applied.
        else if (isUsingAlternatingUpdates() && !_ssch.isCurrentStateFresh()) {
            _ssch.alignToDelay(_pacemakerPulse.delay);
        }

        if (_deferredState.has_value()) {
            _ssch.putNewState(SchedulerPair{_deferredState->state, DummyStatus::active()}, _deferredState->delay);
            _deferredState.reset();
        }

        _ssch.scheduleNewStates();
        _ssch.setChainingBlueStatesAllowed(true);
    }

    void _setStateSchedulerDefaultDelay(hg::PZInteger aNewDefaultDelaySteps) override final {
        if (!isMasterObject()) {
            if (isUsingAlternatingUpdates()) {
                aNewDefaultDelaySteps++;
            }
            _ssch.setDefaultDelay(aNewDefaultDelaySteps);
        }
    }

    const taVisibleState& _getLatestState() const {
        return _ssch.getLatestState().visibleState;
    }

public:
    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_applyUpdate(const VisibleState& aNewState, hg::PZInteger aDelaySteps, SyncFlags aFlags) {
        VisibleState stateToSchedule = [this, &aNewState, aFlags]() {
            if constexpr (std::is_base_of<detail::AutodiffStateTag, VisibleState>::value) {
                if (!IsFullStateFlagSet(aFlags)) {
                    VisibleState state = _getLatestState();
                    state.applyDiff(aNewState);
                    return state;
                }
            }
            return aNewState;
        }();

        if (!IsPacemakerPulseFlagSet(aFlags)) {
        // NORMAL UPDATE
            if (!isUsingAlternatingUpdates()) {
                _ssch.putNewState(SchedulerPair{stateToSchedule, DummyStatus::active()}, aDelaySteps);
            }
            else {
                if (_deferredState.has_value()) {
                    _ssch.putNewState(SchedulerPair{_deferredState->state,  DummyStatus::active()}, _deferredState->delay);
                    _deferredState.reset();
                }

                _ssch.putNewState(SchedulerPair{stateToSchedule, DummyStatus::active()}, aDelaySteps);

                _deferredState = {stateToSchedule, aDelaySteps};
            }
            if (IsNoChainFlagSet(aFlags)) {
                _ssch.setChainingBlueStatesAllowed(false);
            }
            // Save the delay even if the server didn't send the pacemaker pulse:
            // sometimes dummies with alternating updates can trigger 'pacemaking'
            // themselves to get out of degenerate states, and then they will need
            // to know the delay.
            //if (_pacemakerPulse.happened) {
                // This is in the correct place because the pacemaker 
                // pulse actually affects the *following* update.
                _pacemakerPulse.delay = aDelaySteps;
            //}
        } else {
        // PACEMAKER UPDATE
            if (!isUsingAlternatingUpdates()) {
                _ssch.putNewState(SchedulerPair{stateToSchedule, DummyStatus::active()}, aDelaySteps);
            }
            else {
                // When using alternating updates, a pacemaker pulse happens inbetween actual updates
            }
            _pacemakerPulse.happened = true;
            _pacemakerPulse.delay = aDelaySteps;
        }
    }

    //! \warning Internal implementation, do not call in user code!
    void __spempeimpl_deactivateSelfIn(hg::PZInteger aDelaySteps) override final {
        _ssch.putNewState(SchedulerPair{{}, DummyStatus::deactivated()}, aDelaySteps);
    }
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_GAME_OBJECT_FRAMEWORK_GAME_OBJECT_BASES_HPP

// clang-format on
