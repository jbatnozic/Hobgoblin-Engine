
#include "Game_stage_controller.hpp"

#include "Main_gameplay_manager_interface.hpp"

GameStageController::GameStageController(QAO_RuntimeRef  aRuntimeRef,
                                         spe::RegistryId aRegId,
                                         spe::SyncId     aSyncId)
    : SyncObjSuper{aRuntimeRef,
                   SPEMPE_TYPEID_SELF,
                   PRIORITY_GAMESTAGECTRL,
                   "GameStageController",
                   aRegId,
                   aSyncId} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror();
        HG_LOG_INFO(LOG_ID, "GameStageController created.");
    }
}

GameStageController::~GameStageController() {
    if (isMasterObject()) {
        doSyncDestroy();
        HG_LOG_INFO(LOG_ID, "GameStageController destroyed.");
    }
}

void GameStageController::init() {
    auto& self = _getCurrentState();
    self.stage = GAME_STAGE_INITIAL_COUNTDOWN;
    self.count = GAME_STAGE_DURATIONS[self.stage] + 5;
}

int GameStageController::getCurrentGameStage() const {
    if (isDeactivated()) {
        return GAME_STAGE_UNKNOWN;
    }
    return _getCurrentState().stage;
}

void GameStageController::setCurrentGameStage(int aGameStage) {
    auto& self = _getCurrentState();
    self.stage = aGameStage;
}

void GameStageController::_addAnnouncement(const std::string& aString, hg::gr::Color aColor) {
    ccomp<MainGameplayManagerInterface>().addAnnouncement(aString, aColor);
}

void GameStageController::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    auto& self = _getCurrentState();

    switch (self.stage) {
    case GAME_STAGE_INITIAL_COUNTDOWN:
    case GAME_STAGE_RUNNING:
    case GAME_STAGE_HIDE_N_SEEK:
    case GAME_STAGE_PEARL_HUNTING:
    case GAME_STAGE_FINAL:
        self.count -= 1;
        if (self.count == 0) {
            self.stage += 1;
            self.count = GAME_STAGE_DURATIONS[self.stage];
        }
        break;

    case GAME_STAGE_FINISHED:
        {
            // TODO
        }
        break;
    }
}

void GameStageController::_eventUpdate1(spe::IfDummy) {
    if (isDeactivated() || ctx().getGameState().isPaused) {
        return;
    }

    auto& self = _getCurrentState();

    switch (self.stage) {
    case GAME_STAGE_INITIAL_COUNTDOWN:
        if (self.count % 60 == 0) {
            _addAnnouncement(std::to_string(self.count / 60), hg::gr::COLOR_YELLOW);
        }
        break;

    case GAME_STAGE_RUNNING:
        if (self.count == GAME_STAGE_DURATIONS[self.stage]) {
            _addAnnouncement("Go!", hg::gr::COLOR_GREEN);
        }
        break;

    case GAME_STAGE_HIDE_N_SEEK:
        if (self.count == GAME_STAGE_DURATIONS[self.stage]) {
            _addAnnouncement("The Kraken awakes!", hg::gr::COLOR_ORANGE);
        }
        break;

    case GAME_STAGE_PEARL_HUNTING:
        if (self.count == GAME_STAGE_DURATIONS[self.stage]) {
            _addAnnouncement("The Pearls have appeared!", hg::gr::COLOR_ORANGE);
        }
        break;

    case GAME_STAGE_FINAL:
        if (self.count == GAME_STAGE_DURATIONS[self.stage]) {
            _addAnnouncement("Oxygen is running out!", hg::gr::COLOR_ORANGE);
        }
        break;

    case GAME_STAGE_FINISHED:
        {
            // TODO
        }
        break;

    default:
        break;
    }
}

void GameStageController::_eventDrawGUI() {}

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(GameStageController, (CREATE, UPDATE, DESTROY));

void GameStageController::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(GameStageController, aSyncCtrl);
}

void GameStageController::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(GameStageController, aSyncCtrl);
}

void GameStageController::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(GameStageController, aSyncCtrl);
}
