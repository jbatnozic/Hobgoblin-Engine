#include "GameObjects/Pearl.hpp"

#include "GameObjects/Diver.hpp"
#include "Managers/Environment_manager_interface.hpp"
#include "Managers/Main_gameplay_manager_interface.hpp"
#include "Managers/Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"

#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Math.hpp>
#include <iostream>

namespace {
// Physics parameters for Shark
constexpr cpFloat PHYS_MASS    = 2.0;
constexpr cpFloat PHYS_RADIUS  = 8.0;
constexpr cpFloat PHYS_DAMPING = 0.90;
const cpVect      PHYS_GRAVITY = cpv(0.0, 00.0);
} // namespace

Pearl::Pearl(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_LOOT, "Pearl", aRegId, aSyncId} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror();
    } else {
        auto& resMgr = ccomp<ResourceManagerInterface>();
        _sprite      = resMgr.getSpriteLoader().getMultiBlueprint(SPR_PEARL).multispr();
    }
    _enableAlternatingUpdates();

    _gameContext = &ctx();
}

Pearl::~Pearl() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void Pearl::init(float aX, float aY) {
    assert(isMasterObject());

    auto& self = _getCurrentState();
    self.x     = aX;
    self.y     = aY;

    _unibody.init(
        [this]() {
            return _initColDelegate();
        },
        [this]() {
            return hg::alvin::Body::createDynamic(
                PHYS_MASS,
                cpMomentForCircle(PHYS_MASS, 0.0, PHYS_RADIUS, cpvzero));
        },
        [this]() {
            return hg::alvin::Shape::createCircle(_unibody.body, PHYS_RADIUS, cpvzero);
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

hg::alvin::CollisionDelegate Pearl::_initColDelegate() {
    auto builder = hg::alvin::CollisionDelegateBuilder{};
    builder.setDefaultDecision(hg::alvin::Decision::ACCEPT_COLLISION);

    builder.addInteraction<DiverInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](DiverInterface& aDiver, const hg::alvin::CollisionData& aCollisionData) {
            auto* diver = static_cast<Diver*>(&aDiver);
            if (!_deposited && diver->isAlive() && diver->_pearl == nullptr) {
                diver->_pearl = this;
                _holder       = diver;
            }
            return hg::alvin::Decision::REJECT_COLLISION;
        });

    builder.addInteraction<hg::alvin::EntityBase>(
        hg::alvin::COLLISION_PRE_SOLVE,
        [this](hg::alvin::EntityBase& aEntity, const hg::alvin::CollisionData& aCollisionData) {
            if (_deposited || _holder != nullptr) {
                return hg::alvin::Decision::REJECT_COLLISION;
            }
            return hg::alvin::Decision::ACCEPT_COLLISION;
        });

    return builder.finalize();
}

// MARK: QAO Events

void Pearl::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    if (!_deposited && cpBodyGetPosition(_unibody).y < 100.0) {
        _deposited = true;
        if (_holder != nullptr) {
            _holder->_pearl = nullptr;
            _holder         = nullptr;
        }
        ccomp<MainGameplayManagerInterface>().depositPearl();
    }

    if (_deposited) {
        cpBodySetPosition(_unibody, cpv(0.0, -10000.0));
    } else if (_holder != nullptr) {
        cpBodySetPosition(_unibody, cpBodyGetPosition(_holder->_unibody));
    }

    const auto pos = cpBodyGetPosition(_unibody);

    auto& self = _getCurrentState();
    self.x     = (float)pos.x;
    self.y     = (float)pos.y;
}

void Pearl::_eventPostUpdate(spe::IfMaster) {
    if (_didAlternatingUpdatesSync()) {
        _getCurrentState().commit();
    }
}

void Pearl::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    const auto& self_curr = _getCurrentState();
    const auto& self_next = _getFollowingState();

    _sprite.setPosition({(self_curr.x + self_next.x) / 2.f, (self_curr.y + self_next.y) / 2.f});
    ccomp<MWindow>().getCanvas().draw(_sprite);
}

// MARK: Netcode

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(Pearl, (CREATE, UPDATE, DESTROY));

void Pearl::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(Pearl, aSyncCtrl);
}

void Pearl::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
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
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(Pearl, aSyncCtrl);
}

void Pearl::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(Pearl, aSyncCtrl);
}
