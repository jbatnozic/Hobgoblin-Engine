#pragma once

#include "Engine.hpp"
#include "Game_stage.hpp"

#include <string>

// clang-format off
SPEMPE_DEFINE_AUTODIFF_STATE(GameStageController_VisibleState,
    SPEMPE_MEMBER(std::int8_t, stage, 0),
    SPEMPE_MEMBER(std::int16_t, count, 0)) {
};
// clang-format on

class GameStageController
    : public spe::SynchronizedObject<GameStageController_VisibleState> {
public:
    GameStageController(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId);

    ~GameStageController() override;

    void init();

    int getCurrentGameStage() const;
    void setCurrentGameStage(int aGameStage);

private:
    void _addAnnouncement(const std::string& aString, hg::gr::Color aColor);

    void _eventUpdate1(spe::IfMaster) override;
    void _eventUpdate1(spe::IfDummy) override;
    void _eventDrawGUI() override;

    void _syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
    void _syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const override;
};
