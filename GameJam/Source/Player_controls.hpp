#pragma once

#include "Engine.hpp"

using ControlDirectionType = bool;

inline constexpr auto CTRL_ID_UP    = "up";
inline constexpr auto CTRL_ID_DOWN  = "down";
inline constexpr auto CTRL_ID_LEFT  = "left";
inline constexpr auto CTRL_ID_RIGHT = "right";
inline constexpr auto CTRL_ID_JUMP  = "jump";
inline constexpr auto CTRL_ID_START = "start";

inline void SetUpPlayerControls(spe::InputSyncManagerInterface& aInputMgr) {
    spe::InputSyncManagerWrapper wrap{aInputMgr};
    wrap.defineSignal<ControlDirectionType>(CTRL_ID_UP, false);
    wrap.defineSignal<ControlDirectionType>(CTRL_ID_DOWN, false);
    wrap.defineSignal<ControlDirectionType>(CTRL_ID_LEFT, false);
    wrap.defineSignal<ControlDirectionType>(CTRL_ID_RIGHT, false);
    wrap.defineSimpleEvent(CTRL_ID_JUMP);
    wrap.defineSimpleEvent(CTRL_ID_START);
}
