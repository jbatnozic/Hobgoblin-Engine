#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>

#include <cstdint>
#include <memory>
#include <optional>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Diver_VisibleState,
    SPEMPE_MEMBER(std::int32_t, owningPlayerIndex, spe::PLAYER_INDEX_UNKNOWN),
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(float, directionInRad, 0.f),
    SPEMPE_MEMBER(float, oxygen, 100.f)) {
};
// clang-format on

class /* Holy */ Diver // You've been down too long in the midnight sea...
    : public spe::SynchronizedObject<Diver_VisibleState>
    , public DiverInterface {
public:
    Diver(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Diver() override;

    void init(int aOwningPlayerIndex, float aX, float aY);

    void kill() override;

    int getOwningPlayerIndex() const {
        return _getCurrentState().owningPlayerIndex;
    }

private:
    hg::alvin::Unibody _unibody;

    hg::alvin::CollisionDelegate _initColDelegate();

    void _execMovement(bool aLeft, bool aRight, bool aUp, bool aDown);

    void _adjustView();

    void _eventUpdate1(spe::IfMaster) override;
    void _eventUpdate1(spe::IfDummy) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    void _eventDraw2() override;
    void _eventDrawGUI() override;

    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
