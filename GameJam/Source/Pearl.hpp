#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <cstdint>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Pearl_VisibleState,
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f)
) {};
// clang-format on

class Diver;

class Pearl
    : public spe::SynchronizedObject<Pearl_VisibleState>
    , public LootInterface
{
public:
    Pearl(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Pearl() override;

    void init(float aX, float aY);

private:
    spe::GameContext* _gameContext;

    hg::gr::Multisprite _sprite;

    hg::alvin::Unibody _unibody;

    friend class Diver;
    Diver* _holder = nullptr;

    bool _deposited = false;

    hg::alvin::CollisionDelegate _initColDelegate();

    void _eventUpdate1(spe::IfMaster) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    
    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
