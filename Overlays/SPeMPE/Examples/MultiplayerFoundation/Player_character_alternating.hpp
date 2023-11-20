#pragma once

#include "Engine.h"

#include <Hobgoblin/Utility/Autopack.hpp>

#include <cstdint>

struct AlternatingPlayerCharacter_VisibleState {
    float x = 0.f, y = 0.f;
    std::int32_t owningPlayerIndex = spe::PLAYER_INDEX_UNKNOWN;

    HG_ENABLE_AUTOPACK(AlternatingPlayerCharacter_VisibleState, x, y, owningPlayerIndex);
};

/**
 * Implementation of a synchronized object with alternating updates optimization enabled.
 * (the updates are sent every other frame, and then the clients interpolates)
 */
class AlternatingPlayerCharacter
    : public spe::SynchronizedObject<AlternatingPlayerCharacter_VisibleState>
{
public:
    AlternatingPlayerCharacter(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~AlternatingPlayerCharacter() override;

    void init(int aOwningPlayerIndex, float aX, float aY);

private:
    void _eventUpdate1(spe::IfMaster) override;
    void _eventDraw1() override;

    void _syncCreateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncUpdateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncDestroyImpl(spe::SyncDetails& aSyncDetails) const override;
};
