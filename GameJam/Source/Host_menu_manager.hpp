#pragma once

#include "Engine.hpp"

#include "Context_factory.hpp"
#include "Host_menu_manager_interface.hpp"

#include <memory>

class HostMenuManager
    : public HostMenuManagerInterface
    , public spe::NonstateObject {
public:
    HostMenuManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);
    ~HostMenuManager() override;

    void setVisible(bool aVisible) override;

    void setZeroTierEnabled(bool aEnabled) override {
        _zeroTierEnabled = aEnabled;
    }

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> _impl;

    bool                              _zeroTierEnabled = false;
    std::unique_ptr<ClientGameParams> _clientGameParams;
    std::unique_ptr<ServerGameParams> _serverGameParams;
    bool                              _timeToDie = false;

    void _eventPreUpdate() override;
    void _eventDrawGUI() override;
};
