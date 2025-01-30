#include "GameObjects/Shell.hpp"

#include "GameObjects/Bubble.hpp"
#include "GameObjects/Pearl.hpp"
#include "Managers/Environment_manager_interface.hpp"
#include "Managers/Main_gameplay_manager_interface.hpp"
#include "Managers/Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"

#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Math.hpp>
#include <iostream>

Shell::Shell(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR, "Shell", aRegId, aSyncId} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror();
    } else {
        auto& resMgr = ccomp<ResourceManagerInterface>();
        _sprite      = resMgr.getSpriteLoader().getMultiBlueprint(SPR_SHELL).multispr();
    }

    _gameContext = &ctx();
}

Shell::~Shell() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void Shell::init(float aX, float aY) {
    assert(isMasterObject());

    auto& self = _getCurrentState();
    self.x     = aX;
    self.y     = aY;
}

// MARK: QAO Events

void Shell::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    auto&      self      = _getCurrentState();
    const auto gameStage = ccomp<MainGameplayManagerInterface>().getCurrentGameStage();
    if (!self.didSpawn && gameStage == GAME_STAGE_PEARL_HUNTING) {
        self.didSpawn = true;

        auto* p = QAO_PCreate<Pearl>(ctx().getQAORuntime(),
                                     ccomp<MNetworking>().getRegistryId(),
                                     spe::SYNC_ID_NEW);
        p->init(self.x, self.y);
    }
}

void Shell::_eventPostUpdate(spe::IfMaster) {
    _getCurrentState().commit();
}

void Shell::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    const auto& self = _getCurrentState();

    _sprite.setPosition({self.x, self.y});
    ccomp<MWindow>().getCanvas().draw(_sprite);
}

// MARK: Netcode

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(Shell, (CREATE, UPDATE, DESTROY));

void Shell::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(Shell, aSyncCtrl);
}

void Shell::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
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
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(Shell, aSyncCtrl);
}

void Shell::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(Shell, aSyncCtrl);
}
