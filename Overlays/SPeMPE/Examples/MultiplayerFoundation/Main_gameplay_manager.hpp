#pragma once

#include "Engine.h"
#include "Main_gameplay_manager_interface.hpp"

class MainGameplayManager
    : public  MainGameplayManagerInterface
    , public  spe::NonstateObject
    , private spe::NetworkingEventListener {
public:
    explicit MainGameplayManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority);

    ~MainGameplayManager() override;

private:
    hg::PZInteger stateBufferingLength = 0;

    hg::PZInteger printBandwidthUsageCountdown = 120;

    void _eventUpdate1() override;
    void _eventDrawGUI() override;
    void _eventFinalizeFrame() override;

    void onNetworkingEvent(const RN_Event& aEvent) override;
};
