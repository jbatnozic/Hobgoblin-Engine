#pragma once

#include "Engine.hpp"

struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;

    bool hold;
    bool start;

    float mouse_x;
    float mouse_y;
};

inline void SetUpPlayerControls(spe::InputSyncManagerInterface& aInputMgr) {
    spe::InputSyncManagerWrapper input{aInputMgr};

    input.defineSignal<bool>("up", false);
    input.defineSignal<bool>("down", false);
    input.defineSignal<bool>("left", false);
    input.defineSignal<bool>("right", false);

    input.defineSignal<bool>("hold", false);
    input.defineSimpleEvent("start");

    input.defineSignal("mouse_x", 0.f);
    input.defineSignal("mouse_y", 0.f);
}

inline void SetPlayerInput(spe::InputSyncManagerInterface& aInputMgr, const PlayerInput& aPlayerInput) {
    spe::InputSyncManagerWrapper input{aInputMgr};

    // clang-format off
    input.setSignalValue("up",    aPlayerInput.up);
    input.setSignalValue("down",  aPlayerInput.down);
    input.setSignalValue("left",  aPlayerInput.left);
    input.setSignalValue("right", aPlayerInput.right);

    input.setSignalValue("hold", aPlayerInput.hold);
    input.triggerEvent("start", aPlayerInput.start);

    input.setSignalValue("mouse_x", aPlayerInput.mouse_x);
    input.setSignalValue("mouse_y", aPlayerInput.mouse_y);
    // clang-format on
}

inline PlayerInput GetPlayerInput(spe::InputSyncManagerInterface& aInputMgr, int aClientIndex) {
    PlayerInput result;

    spe::InputSyncManagerWrapper input{aInputMgr};

    result.up    = input.getSignalValue<bool>(aClientIndex, "up");
    result.down  = input.getSignalValue<bool>(aClientIndex, "down");
    result.left  = input.getSignalValue<bool>(aClientIndex, "left");
    result.right = input.getSignalValue<bool>(aClientIndex, "right");

    result.hold  = input.getSignalValue<bool>(aClientIndex, "hold");
    result.start = (input.countSimpleEvent(aClientIndex, "start") > 0);

    result.mouse_x = input.getSignalValue<float>(aClientIndex, "mouse_x");
    result.mouse_y = input.getSignalValue<float>(aClientIndex, "mouse_y");

    return result;
}
