#pragma once

#include <Hobgoblin/QAO.hpp>
#include <Hobgoblin/RigelNet.hpp>
#include <SPeMPE/SPeMPE.hpp>

namespace hg = ::jbatnozic::hobgoblin;
namespace spe = ::jbatnozic::spempe;
using namespace hg::qao; // All names from QAO are prefixed with QAO_
using namespace hg::rn;  // All names from RigelNet are prefixed with RN_

using MInput      = spe::InputSyncManagerInterface;
using MLobby      = spe::LobbyManagerInterface;
using MNetworking = spe::NetworkingManagerInterface;
using MWindow     = spe::WindowManagerInterface;

#define PRIORITY_VARMAPMGR     16
#define PRIORITY_NETWORKMGR    15
#define PRIORITY_LOBBYMGR      14
#define PRIORITY_LOBBYFRONTMGR 13
#define PRIORITY_AUTHMGR       12
#define PRIORITY_GAMEPLAYMGR   10
#define PRIORITY_INPUTMGR       7
#define PRIORITY_PLAYERAVATAR   5
#define PRIORITY_WINDOWMGR      0

#define STATE_BUFFERING_LENGTH 2