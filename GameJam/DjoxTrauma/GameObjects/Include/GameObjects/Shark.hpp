#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>

#include <cstdint>
#include <memory>
#include <optional>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Shark_VisibleState,
    SPEMPE_MEMBER(std::int32_t, owningPlayerIndex, spe::PLAYER_INDEX_UNKNOWN),
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(float, directionInRad, 0.f)) {
};
// clang-format on

class Shark
    : public spe::SynchronizedObject<Shark_VisibleState>
    , public SharkInterface {
public:
    Shark(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Shark() override;

    void init(int aOwningPlayerIndex, float aX, float aY);

    int getOwningPlayerIndex() const {
        return _getCurrentState().owningPlayerIndex;
    }

private:
    hg::alvin::Unibody _unibody;

    cpFloat _propulsionForce;

    hg::gr::Multisprite _bodySprite;
    hg::gr::Multisprite _finsSprite;

    hg::alvin::CollisionDelegate _initColDelegate();

    void _execMovement(bool aLeft, bool aRight, bool aUp, bool aDown);

    bool _isInMouth(const cpVect& aPosition) const;

    void _adjustView();

    void _eventUpdate1(spe::IfMaster) override;
    void _eventUpdate1(spe::IfDummy) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    void _eventDraw2() override;

    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
