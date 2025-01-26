#include "Bubble.hpp"

#include "Environment_manager_interface.hpp"
#include "Main_gameplay_manager_interface.hpp"

#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Math.hpp>
#include <iostream>

namespace {
// Physics parameters for Shark
constexpr cpFloat PHYS_MASS    = 0.1;
constexpr cpFloat PHYS_DAMPING = 0.90;
const cpVect      PHYS_GRAVITY = cpv(0.0, -500.0);
} // namespace

Bubble::Bubble(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_LOOT, "Bubble", aRegId, aSyncId} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror();
    }
    _enableAlternatingUpdates();

    _gameContext = &ctx();
}

Bubble::~Bubble() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void Bubble::init(float                        aOxygen,
                  float                        aX,
                  float                        aY,
                  float                        aRadius,
                  const hg::alvin::EntityBase* aOwner) {
    assert(isMasterObject());

    _oxygen = aOxygen;
    _owner = aOwner;

    auto& self  = _getCurrentState();
    self.x      = aX;
    self.y      = aY;
    self.radius = aRadius;

    _unibody.init(
        [this, aRadius]() {
            return _initColDelegate();
        },
        [this, aRadius]() {
            return hg::alvin::Body::createDynamic(PHYS_MASS,
                                                  cpMomentForCircle(PHYS_MASS, 0.0, aRadius, cpvzero));
        },
        [this, aRadius]() {
            return hg::alvin::Shape::createCircle(_unibody.body, aRadius, cpvzero);
        });

    cpBodySetVelocityUpdateFunc(
        _unibody,
        [](cpBody* aBody, cpVect /*aGravity*/, cpFloat /*aDamping*/, cpFloat aDt) {
            cpBodyUpdateVelocity(aBody, PHYS_GRAVITY, PHYS_DAMPING, aDt);
        });

    _unibody.bindDelegate(*this);
    _unibody.addToSpace(ccomp<MEnvironment>().getSpace());

    cpBodySetPosition(_unibody, cpv(aX, aY));
}

hg::alvin::CollisionDelegate Bubble::_initColDelegate() {
    auto builder = hg::alvin::CollisionDelegateBuilder{};
    builder.setDefaultDecision(hg::alvin::Decision::ACCEPT_COLLISION);

    builder.addInteraction<TerrainInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](TerrainInterface& aTerrain, const hg::alvin::CollisionData& aCollisionData) {
            _pop = true;
            return hg::alvin::Decision::ACCEPT_COLLISION;
        });

    builder.addInteraction<DiverInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](DiverInterface& aDiver, const hg::alvin::CollisionData& aCollisionData) {
            if (&aDiver == _owner) {
                _owner = nullptr;
                return hg::alvin::Decision::REJECT_COLLISION;
            }
            if (!_pop) {
                aDiver.addOxygen(_oxygen);
                _pop = true;
            }
            return hg::alvin::Decision::REJECT_COLLISION;
        });

    builder.addInteraction<SharkInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](SharkInterface& aShark, const hg::alvin::CollisionData& aCollisionData) {
            _pop = true;
            return hg::alvin::Decision::REJECT_COLLISION;
        });

    return builder.finalize();
}

// MARK: QAO Events

void Bubble::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    const auto pos = cpBodyGetPosition(_unibody);

    auto& self = _getCurrentState();
    self.x     = (float)pos.x;
    self.y     = (float)pos.y;

    if (_pop) {
        QAO_PDestroy(this);
    }
}

void Bubble::_eventPostUpdate(spe::IfMaster) {
    if (_didAlternatingUpdatesSync()) {
        _getCurrentState().commit();
    }
}

void Bubble::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    const auto& self_curr = _getCurrentState();
    const auto& self_next = _getFollowingState();

    hg::gr::CircleShape circle{self_curr.radius};
    circle.setOrigin({self_curr.radius, self_curr.radius});
    circle.setFillColor(hg::gr::COLOR_TRANSPARENT);
    circle.setOutlineColor(hg::gr::COLOR_AQUAMARINE);
    circle.setOutlineThickness(1.f);
    circle.setPosition({(self_curr.x + self_next.x) / 2.f, (self_curr.y + self_next.y) / 2.f});
    ccomp<MWindow>().getCanvas().draw(circle);
}

// MARK: Netcode

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(Bubble, (CREATE, UPDATE, DESTROY));

void Bubble::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(Bubble, aSyncCtrl);
}

void Bubble::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    auto&      gpMgr   = _gameContext->getComponent<MainGameplayManagerInterface>();
    const auto selfPos = cpBodyGetPosition(_unibody);
    aSyncCtrl.filter([this, &gpMgr, &selfPos](hg::PZInteger aClientIndex) {
        const auto playerPos = gpMgr.getPositionOfClient(aClientIndex);
        if (!playerPos.has_value()) {
            return spe::SyncFilterStatus::REGULAR_SYNC;
        }
        if (hg::math::EuclideanDist(selfPos.x, selfPos.y, playerPos->x, playerPos->y) < 1000.0) {
            return spe::SyncFilterStatus::REGULAR_SYNC;
        }
        return spe::SyncFilterStatus::DEACTIVATE;
    });
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(Bubble, aSyncCtrl);
}

void Bubble::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(Bubble, aSyncCtrl);
}
