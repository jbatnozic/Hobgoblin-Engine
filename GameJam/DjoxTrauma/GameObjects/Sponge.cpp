#include "GameObjects/Sponge.hpp"

#include "GameObjects/Bubble.hpp"
#include "Managers/Environment_manager_interface.hpp"
#include "Managers/Main_gameplay_manager_interface.hpp"
#include "Managers/Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"

#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Math.hpp>
#include <iostream>

Sponge::Sponge(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR, "Sponge", aRegId, aSyncId} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror();
    } else {
        auto& resMgr = ccomp<ResourceManagerInterface>();
        _sprite = resMgr.getSpriteLoader().getMultiBlueprint(SPR_SPONGE).multispr();
    }

    _gameContext = &ctx();
}

Sponge::~Sponge() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void Sponge::init(float aX, float aY) {
    assert(isMasterObject());

    auto& self = _getCurrentState();
    self.x     = aX;
    self.y     = aY;
}

// MARK: QAO Events

void Sponge::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    const auto gameStage = ccomp<MainGameplayManagerInterface>().getCurrentGameStage();
    if (gameStage >= GAME_STAGE_FINAL) {
        return;
    }

    _bubbleSpawnCooldown -= 1;
    if (_bubbleSpawnCooldown == 0) {
        auto& self = _getCurrentState();
        auto* obj  = QAO_PCreate<Bubble>(ctx().getQAORuntime(),
                                        ccomp<MNetworking>().getRegistryId(),
                                        spe::SYNC_ID_NEW);
        obj->init(10.f, self.x, self.y, 8.f, nullptr);

        _bubbleSpawnCooldown = BUBBLE_SPAWN_COOLDOWN;
    }
}

void Sponge::_eventPostUpdate(spe::IfMaster) {
    _getCurrentState().commit();
}

void Sponge::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    const auto& self = _getCurrentState();

    _sprite.setPosition({self.x, self.y});
    ccomp<MWindow>().getCanvas().draw(_sprite);
}

// MARK: Netcode

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(Sponge, (CREATE, UPDATE, DESTROY));

void Sponge::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(Sponge, aSyncCtrl);
}

void Sponge::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    auto& gpMgr = _gameContext->getComponent<MainGameplayManagerInterface>();
    auto& self  = _getCurrentState();
    aSyncCtrl.filter([this, &gpMgr, &self](hg::PZInteger aClientIndex) {
        const auto playerPos = gpMgr.getPositionOfClient(aClientIndex);
        if (!playerPos.has_value()) {
            return spe::SyncFilterStatus::REGULAR_SYNC;
        }
        if (hg::math::EuclideanDist<double>(self.x, self.y, playerPos->x, playerPos->y) < 1000.0) {
            return spe::SyncFilterStatus::REGULAR_SYNC;
        }
        return spe::SyncFilterStatus::DEACTIVATE;
    });
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(Sponge, aSyncCtrl);
}

void Sponge::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(Sponge, aSyncCtrl);
}
