#pragma once

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/QAO.hpp>
#include <Hobgoblin/RigelNet.hpp>
#include <SPeMPE/SPeMPE.hpp>

namespace hg  = ::jbatnozic::hobgoblin;
namespace spe = ::jbatnozic::spempe;
using namespace hg::qao; // All names from QAO are prefixed with QAO_
using namespace hg::rn;  // All names from RigelNet are prefixed with RN_

using hg::AvoidNull;
using hg::NeverNull;

inline constexpr auto LOG_ID = "GMTK2024";

using MInput        = spe::InputSyncManagerInterface;
using MLobbyBackend = spe::LobbyBackendManagerInterface;
using MNetworking   = spe::NetworkingManagerInterface;
using MWindow       = spe::WindowManagerInterface;
using MVarmap       = spe::SyncedVarmapManagerInterface;

#define PRIORITY_RESOURCEMGR    21
#define PRIORITY_MAINMENUMGR    20
#define PRIORITY_HOSTMENUMGR    20
#define PRIORITY_JOINMENUMGR    20
#define PRIORITY_VARMAPMGR      16
#define PRIORITY_NETWORKMGR     15
#define PRIORITY_ENVIRONMENTMGR 14
#define PRIORITY_LOBBYBACKMGR   14
#define PRIORITY_LOBBYFRONTMGR  13
#define PRIORITY_AUTHMGR        12
#define PRIORITY_GAMESTAGECTRL  11
#define PRIORITY_GAMEPLAYMGR    10
#define PRIORITY_INPUTMGR       7
#define PRIORITY_LOOT           6
#define PRIORITY_PLAYERAVATAR   5
#define PRIORITY_WINDOWMGR      0
