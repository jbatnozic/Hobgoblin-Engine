
#include <iostream>
#include <string>

#include "Main_menu.hpp"
#include "../Control/Environment_manager.hpp"
//#include "../Gameplay/Physics_player.hpp"

#include <Ztcpp.hpp>

namespace zt = jbatnozic::ztcpp;

class NodeInfo {
public:
  NodeInfo() = default;

  bool online = false;
  int networksJoinedCount = 0;
  uint64_t id = 0;
  zt::IpAddress ip4 = zt::IpAddress::ipv4Unspecified();
  zt::IpAddress ip6 = zt::IpAddress::ipv6Unspecified();
};

extern NodeInfo localNode;

namespace {

template <class T>
T InputPrompt(const std::string& name, T defaultValue) {
    std::cout << "Input " << name << " (press Enter for default - " << defaultValue << "): ";
    std::string temp;
    std::getline(std::cin, temp);
    if (temp.empty()) {
        return defaultValue;
    }
    else {
        std::stringstream ss;
        ss << temp;
        T t;
        ss >> t;
        return t;
    }
}

} // namespace

MainMenu::MainMenu(QAO_RuntimeRef runtimeRef)
    : NonstateObject{runtimeRef, TYPEID_SELF, 0, "MainMenu"}
{
}

void MainMenu::_eventUpdate1() {
    // TODO - Horrible magic method... Refactor this shit...
#define HOST 1
#define CLIENT 2
#define SOLO 3
#define GAME_MASTER 4

  int mode = InputPrompt<int>("mode - 1 = host; 2 = client, 3 = solo, 4 = GM", 2);
    if (mode == HOST) {
        // Start a local server in the background:
        auto serverCtx = std::make_unique<GameContext>(ctx().getResourceConfig(), 
                                                       ctx().getRuntimeConfig());
        ExtendGameContext(*serverCtx);

        const std::uint16_t localPort = InputPrompt<std::uint16_t>("local port - 0 for any", 8888);
        const hg::PZInteger clientCount = InputPrompt<hg::PZInteger>("client count", 2);

        serverCtx->configure(GameContext::Mode::Server);
        serverCtx->getNetworkingManager().getServer().start(localPort);
        serverCtx->getNetworkingManager().getServer().resize(clientCount);
        serverCtx->getNetworkingManager().getServer().setTimeoutLimit(std::chrono::seconds{5});

        //const std::uint16_t serverPort = serverCtx->getNetworkingManager().getServer().getLocalPort();
        auto& server = serverCtx->getNetworkingManager().getServer();

        generateTerrain(*serverCtx);

        // Connect to the server:
        ctx().configure(GameContext::Mode::Client);
        std::cout << "Connecting to self (IP = " << sf::IpAddress::LocalHost << ")\n";
        // ctx(MNetworking).getClient().connect(0, sf::IpAddress::LocalHost, serverPort);
        ctx(MNetworking).getClient().connectLocal(server);
        ctx(MNetworking).getClient().setTimeoutLimit(std::chrono::seconds{5});



        ctx().runChildContext(std::move(serverCtx));
    }
    else if (mode == CLIENT) {
        const std::uint16_t localPort  = InputPrompt<std::uint16_t>("local port", 0);
        const std::string   serverIp   = InputPrompt<std::string>("server IP", "127.0.0.1");
        const std::uint16_t serverPort = InputPrompt<std::uint16_t>("server port", 8888);

        ctx().configure(GameContext::Mode::Client);
        ctx(MNetworking).getClient().connect(localPort, serverIp, serverPort);
        ctx(MNetworking).getClient().setTimeoutLimit(std::chrono::seconds{5});
    }
    else if (mode == SOLO) {
        ctx().configure(GameContext::Mode::Solo);
        generateTerrain(ctx());

        //PhysicsPlayer::VisibleState vs;
        //vs.playerIndex = 0;
        //vs.x = 70.f;
        //vs.y = 70.f;
        //QAO_PCreate<PhysicsPlayer>(getRuntime(), ctx().getSyncObjReg(), SYNC_ID_NEW, vs);
    }
    else if (mode == GAME_MASTER) {
        ctx().configure(GameContext::Mode::GameMaster);
        ctx(MNetworking).getServer().start(InputPrompt<std::uint16_t>("local port - 0 for any", 8888));
        ctx(MNetworking).getServer().resize(InputPrompt<hg::PZInteger>("client count", 2));
        ctx(MNetworking).getServer().setTimeoutLimit(std::chrono::seconds{5});

        generateTerrain(ctx());
    }

    QAO_PDestroy(this);

#undef HOST 
#undef CLIENT 
#undef SOLO 
#undef GAME_MASTER 
}

void MainMenu::generateTerrain(GameContext& ctx) {
    std::cout << "Generating terrain...\n";
    hg::util::Stopwatch stopwatch;
    GetEnvironmentManager(ctx).generate(100, 100, 32.f);
    std::cout << "DONE! Terrain generated (took " << stopwatch.getElapsedTime().count() << "ms)\n";
}
