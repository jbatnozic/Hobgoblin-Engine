#pragma once

#include "Collisions.hpp"
#include "Engine.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Graphics.hpp>

enum class LootKind : std::int8_t {
    NONE,
    PROTEIN
};

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(LootObject_VisibleState,
    SPEMPE_MEMBER(float, x, 0.f),
    SPEMPE_MEMBER(float, y, 0.f),
    SPEMPE_MEMBER(std::int8_t, kind, (std::int8_t)LootKind::NONE)) {
};
// clang-format on

class LootObject
    : public spe::SynchronizedObject<LootObject_VisibleState>
    , public LootInterface {
public:
    LootObject(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~LootObject() override;

    void init(float aX, float aY, LootKind aKind);

private:
    hg::alvin::Unibody _unibody;

    hg::gr::Multisprite _spr;

    float _wiggleAngle = 0.f;

    hg::alvin::CollisionDelegate _initColDelegate();

    // void _eventUpdate1(spe::IfMaster) override;
    void _eventUpdate1(spe::IfDummy) override;
    void _eventPostUpdate(spe::IfMaster) override;
    void _eventDraw1() override;
    void _eventDraw2() override;

    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
