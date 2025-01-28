#pragma once

#include "Engine.hpp"

#include <cstdint>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Sponge_VisibleState,
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f)
) {};
// clang-format on

class Sponge
    : public spe::SynchronizedObject<Sponge_VisibleState>
{
public:
    Sponge(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Sponge() override;

    void init(float aX, float aY);

private:
    spe::GameContext* _gameContext;

    hg::gr::Multisprite _sprite;

    static constexpr hg::PZInteger BUBBLE_SPAWN_COOLDOWN = 20;

    hg::PZInteger _bubbleSpawnCooldown = BUBBLE_SPAWN_COOLDOWN;

    void _eventUpdate1(spe::IfMaster) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    
    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
