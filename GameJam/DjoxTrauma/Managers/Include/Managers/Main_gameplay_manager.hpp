#pragma once

#include "Engine.hpp"
#include "Managers/Main_gameplay_manager_interface.hpp"

#include <Hobgoblin/Alvin.hpp>

#include <memory>
#include <string>
#include <vector>

class GameStageController;

class MainGameplayManager
    : public MainGameplayManagerInterface
    , public spe::NonstateObject
    , private spe::NetworkingEventListener {
public:
    explicit MainGameplayManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);

    ~MainGameplayManager() override;

    void setToHostMode(hg::PZInteger aPlayerCount) override;
    void setToClientMode() override;
    Mode getMode() const override;

    void startGame() override;

    void addAnnouncement(const std::string& aString, hg::gr::Color aColor) override;

    int getCurrentGameStage() const override;

    std::optional<cpVect> getPositionOfClient(int aClientIndex) const override;
    void                  setPositionOfClient(int aClientIndex, cpVect aPosition) override;

    void depositPearl() override;

private:
    Mode _mode = Mode::UNINITIALIZED;

    GameStageController* _gameStageController = nullptr;

    // hg::PZInteger stateBufferingLength = 0;
    hg::PZInteger _playerCount;

    class Announcements;
    std::unique_ptr<Announcements> _announcements;

    struct PendingAnnouncement {
        std::string   string;
        hg::gr::Color color;
    };
    std::vector<PendingAnnouncement> _pendingAnnouncements;

    std::vector<cpVect> _playerPositions;

    void _startGame(hg::PZInteger aPlayerCount);
    void _restartGame();
    void _backToMainMenu();

    void _eventUpdate1() override;
    void _eventPostUpdate() override;
    void _eventDrawGUI() override;

    void onNetworkingEvent(const RN_Event& aEvent) override;
};
