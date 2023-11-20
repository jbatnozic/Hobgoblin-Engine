#pragma once

#include "Engine.h"

#include <Hobgoblin/Utility/Autopack.hpp>

#include <cstdint>

struct BasicPlayerCharacter_VisibleState {
    float x = 0.f, y = 0.f;
    std::int32_t owningPlayerIndex = spe::PLAYER_INDEX_UNKNOWN;

    HG_ENABLE_AUTOPACK(BasicPlayerCharacter_VisibleState, x, y, owningPlayerIndex);
};

/**
 * The most basic implementation of a synchronized object, without any particular
 * optimizations as far as bandwidth usage goes.
 */
class BasicPlayerCharacter
    : public spe::SynchronizedObject<BasicPlayerCharacter_VisibleState>
{
public:
    BasicPlayerCharacter(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~BasicPlayerCharacter() override;

    void init(int aOwningPlayerIndex, float aX, float aY);

private:
    void _eventUpdate1(spe::IfMaster) override;
    void _eventDraw1() override;

    void _syncCreateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncUpdateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncDestroyImpl(spe::SyncDetails& aSyncDetails) const override;
};
