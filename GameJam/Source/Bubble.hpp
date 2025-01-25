#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <cstdint>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Bubble_VisibleState,
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(float, radius, 0.f)
) {};
// clang-format on

class Bubble
    : public spe::SynchronizedObject<Bubble_VisibleState>
    , public LootInterface
{
public:
    Bubble(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Bubble() override;

    void init(float aX, float aY, float aRadius);

private:
    spe::GameContext* _gameContext;

    hg::alvin::Unibody _unibody;

    bool _pop = false;

    hg::alvin::CollisionDelegate _initColDelegate();

    void _eventUpdate1(spe::IfMaster) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    
    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
