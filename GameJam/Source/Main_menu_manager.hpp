#pragma once

#include "Engine.hpp"

#include "Context_factory.hpp"
#include "Main_menu_manager_interface.hpp"

#include <memory>

class MainMenuManager
    : public MainMenuManagerInterface
    , public spe::NonstateObject {
public:
    MainMenuManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);
    ~MainMenuManager() override;

    void setVisible(bool aVisible) override;

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> _impl;

    std::unique_ptr<ServerGameParams> _serverGameParams;
    std::unique_ptr<ClientGameParams> _clientGameParams;
    bool                              _timeToDie = false;

    void _eventPreUpdate() override;
    void _eventDrawGUI() override;
};
