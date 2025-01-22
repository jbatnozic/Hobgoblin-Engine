#pragma once

#include "Engine.hpp"
#include "Lobby_frontend_manager_interface.hpp"

#include <memory>

class LobbyFrontendManager
    : public LobbyFrontendManagerInterface
    , public spe::NonstateObject {
public:
    LobbyFrontendManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);
    ~LobbyFrontendManager() override;

    void setToHeadlessHostMode() override;
    void setToClientMode(const std::string& aName, const std::string& aUniqueId) override;
    Mode getMode() const override;

    void setVisible(bool aVisible) override;

private:
    class Impl;
    friend Impl;
    std::unique_ptr<Impl> _impl;

    void _eventBeginUpdate() override;
    void _eventUpdate1() override;
    void _eventDrawGUI() override;

    void _notifyAllToStart();
    friend void ActivateCommand(LobbyFrontendManager& aMgr, int aCommand, void* aArgs);
};
