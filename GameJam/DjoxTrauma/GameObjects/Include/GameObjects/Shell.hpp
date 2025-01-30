#pragma once

#include "Engine.hpp"

#include <cstdint>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(Shell_VisibleState,
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(bool, didSpawn, false)
) {};
// clang-format on

class Shell : public spe::SynchronizedObject<Shell_VisibleState> {
public:
    Shell(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~Shell() override;

    void init(float aX, float aY);

private:
    spe::GameContext* _gameContext;

    hg::gr::Multisprite _sprite;

    void _eventUpdate1(spe::IfMaster) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;

    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
