#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>

#include <cstdint>
#include <memory>
#include <optional>

static constexpr float HOLD_BREATH_MAX = 240.f;

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Diver_VisibleState,
    SPEMPE_MEMBER(std::int32_t, owningPlayerIndex, spe::PLAYER_INDEX_UNKNOWN),
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(float, directionInRad, 0.f),
    SPEMPE_MEMBER(float, oxygen, 100.f),
    SPEMPE_MEMBER(float, breath, HOLD_BREATH_MAX),
    SPEMPE_MEMBER(bool, eaten, false)) {
};
// clang-format on

class Pearl;

class /* Holy */ Diver // You've been down too long in the midnight sea...
    : public spe::SynchronizedObject<Diver_VisibleState>
    , public DiverInterface {
public:
    Diver(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Diver() override;

    void init(int aOwningPlayerIndex, float aX, float aY);

    void addOxygen(float aOxygen) override;

    void kill() override;

    int getOwningPlayerIndex() const {
        return _getCurrentState().owningPlayerIndex;
    }

    bool isAlive() const {
        auto& self = _getCurrentState();
        return (self.oxygen > 0.f && !self.eaten);
    }

private:
    friend class Pearl;
    Pearl* _pearl = nullptr;

    hg::alvin::Unibody _unibody;

    hg::gr::Multisprite _sprite;

    static constexpr hg::PZInteger BUBBLE_SPAWN_COOLDOWN = 30;

    hg::PZInteger _bubbleSpawnCooldown = BUBBLE_SPAWN_COOLDOWN;

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
