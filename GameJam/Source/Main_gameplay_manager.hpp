#pragma once

#include "Engine.hpp"
#include "Main_gameplay_manager_interface.hpp"

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

    void characterReachedTheScales(CharacterObject& aCharacter) override;

    CharacterObject* getContender1() const override {
        return contender1;
    }

    CharacterObject* getContender2() const override {
        return contender2;
    }

private:
    Mode _mode = Mode::UNINITIALIZED;

    // hg::PZInteger stateBufferingLength = 0;
    hg::PZInteger _playerCount;

    CharacterObject* contender1 = nullptr;
    CharacterObject* contender2 = nullptr;

    void _startGame(hg::PZInteger aPlayerCount);
    void _restartGame();
    void _backToMainMenu();

    void _eventUpdate1() override;
    void _eventPostUpdate() override;
    void _eventDrawGUI() override;

    void onNetworkingEvent(const RN_Event& aEvent) override;
};
