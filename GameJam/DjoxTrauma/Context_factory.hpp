#pragma once

#include "Engine.hpp"

#include <cstdint>
#include <memory>
#include <string>

// MARK: Server

struct ServerGameParams {
    hg::PZInteger playerCount; //!< # of players including the host (player 0)
    std::uint16_t portNumber;
    bool          zeroTierEnabled;
};

std::unique_ptr<spe::GameContext> CreateServerContext(const ServerGameParams& aParams);

// MARK: Client

struct ClientGameParams {
    std::string   playerName;
    std::string   hostIpAddress;
    std::uint16_t hostPortNumber;
    std::uint16_t localPortNumber;
    bool          zeroTierEnabled;
    bool          skipConnect;
};

//! Creates a context with only a WindowManager and MainMenuManager
std::unique_ptr<spe::GameContext> CreateBasicClientContext();

void AttachGameplayManagers(spe::GameContext& aContext, const ClientGameParams& aParams);
