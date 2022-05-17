#include <Hobgoblin/Utility/Autopack.hpp>
#include <Hobgoblin/Utility/State_scheduler.hpp>
#include <SPeMPE/SPeMPE.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace hg  = ::jbatnozic::hobgoblin;
namespace spe = ::jbatnozic::spempe;
using namespace hg::qao; // All names from QAO are prefixed with QAO_
using namespace hg::rn;  // All names from RigelNet are prefixed with RN_

using MInput      = spe::InputSyncManagerInterface;
using MNetworking = spe::NetworkingManagerInterface;
using MWindow     = spe::WindowManagerInterface;

#define PRIORITY_VARMAPMGR    16
#define PRIORITY_NETWORKMGR   15
#define PRIPRITY_GAMEPLAYMGR  10
#define PRIORITY_INPUTMGR      7
#define PRIORITY_PLAYERAVATAR  5
#define PRIORITY_WINDOWMGR     0

#define STATE_BUFFERING_LENGTH 2

///////////////////////////////////////////////////////////////////////////
// PLAYER CONTROLS                                                       //
///////////////////////////////////////////////////////////////////////////

struct PlayerControls {
    bool left  = false;
    bool right = false;
    bool up    = false;
    bool down  = false;
    bool jump  = false;
};

///////////////////////////////////////////////////////////////////////////
// MAIN GAMEPLAY CONTROLLER INTERFACE                                    //
///////////////////////////////////////////////////////////////////////////

class GameplayManagerInterface : public spe::ContextComponent {
private:
    SPEMPE_CTXCOMP_TAG("GameplayManagerInterface");
};

using MGameplay = GameplayManagerInterface;

///////////////////////////////////////////////////////////////////////////
// PLAYER AVATARS                                                        //
///////////////////////////////////////////////////////////////////////////

struct PlayerAvatar_VisibleState {
    float x, y;
    int owningPlayerIndex = spe::PLAYER_INDEX_UNKNOWN;
    HG_ENABLE_AUTOPACK(PlayerAvatar_VisibleState, x, y, owningPlayerIndex);
};

class PlayerAvatar : public spe::SynchronizedObject<PlayerAvatar_VisibleState> {
public:
    PlayerAvatar(QAO_RuntimeRef aRuntimeRef,
                 spe::RegistryId aRegId,
                 spe::SyncId aSyncId)
        : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR,
                       "PlayerAvatar", aRegId, aSyncId}
    {
    }

    ~PlayerAvatar() override {
        if (isMasterObject()) {
            // TODO: See if this can be improved.
            // Maybe some default implementation?
            doSyncDestroy();
        }
    }

    void init(int aOwningPlayerIndex) {
        assert(isMasterObject());

        auto& self = _getCurrentState();
        self.x = 400.f;
        self.y = 400.f;
        self.owningPlayerIndex = aOwningPlayerIndex;
    }

private:
    void _eventUpdate(spe::IfMaster) override {
        auto& self = _getCurrentState();
        assert(self.owningPlayerIndex >= 0);

        spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};

        const bool left  = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "left");
        const bool right = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "right");

        self.x += (10.f * ((float)right - (float)left));

        const bool up = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "up");
        const bool down = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "down");

        self.y += (10.f * ((float)down - (float)up));

        wrapper.pollSimpleEvent(self.owningPlayerIndex, "jump",
                                [&]() {
                                    self.y -= 16.f;
                                });
    }

    void _eventDraw1() override {
        if (isDeactivated()) {
            return;
        }

        const auto& self = _getCurrentState();
        
        sf::CircleShape circle{20.f};
        circle.setFillColor(hg::gr::Color::Red);
        circle.setPosition({self.x, self.y});
        ccomp<MWindow>().getCanvas().draw(circle);

        if (!ctx().isHeadless()) {
            auto& winMgr = static_cast<spe::WindowManagerOne&>(ccomp<MWindow>());
            const auto pos = winMgr._getMousePos(0);
            sf::RectangleShape rect;
            rect.setSize({32.f, 32.f});
            rect.setFillColor(hg::gr::Color::AliceBlue);
            rect.setPosition(pos);
            winMgr.getCanvas().draw(rect);
        }
    }

    void _syncCreateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncUpdateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncDestroyImpl(spe::SyncDetails& aSyncDetails) const override;
};

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(PlayerAvatar, (CREATE, UPDATE, DESTROY));

void PlayerAvatar::_syncCreateImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

void PlayerAvatar::_syncUpdateImpl(spe::SyncDetails& aSyncDetails) const {
    aSyncDetails.filterSyncs(
        [](hg::PZInteger aRecepient) -> spe::SyncDetails::FilterResult {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                return spe::SyncDetails::FilterResult::Skip;
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                return spe::SyncDetails::FilterResult::Deactivate;
            }
            return spe::SyncDetails::FilterResult::FullSync;
        }
    );
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

void PlayerAvatar::_syncDestroyImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

///////////////////////////////////////////////////////////////////////////
// SINCLAIRE AVATARS                                                     //
///////////////////////////////////////////////////////////////////////////

struct SinclaireAvatar_VisibleState {
    float x, y;
    int owningPlayerIndex = spe::PLAYER_INDEX_UNKNOWN;
    HG_ENABLE_AUTOPACK(SinclaireAvatar_VisibleState, x, y, owningPlayerIndex);
};

class SinclaireAvatar : public spe::SynchronizedObject<SinclaireAvatar_VisibleState> {
public:
    SinclaireAvatar(QAO_RuntimeRef aRuntimeRef,
                    spe::RegistryId aRegId,
                    spe::SyncId aSyncId)
        : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR,
        "SinclaireAvatar", aRegId, aSyncId}
    {
        _enableAlternatingUpdates();
    }

    ~SinclaireAvatar() override {
        if (isMasterObject()) {
            // TODO: See if this can be improved.
            // Maybe some default implementation?
            doSyncDestroy();
        }
    }

    void init(int aOwningPlayerIndex) {
        assert(isMasterObject());

        auto& self = _getCurrentState();
        self.x = 400.f;
        self.y = 450.f;
        self.owningPlayerIndex = aOwningPlayerIndex;
    }

private:
    void _eventUpdate(spe::IfMaster) override {
        auto& self = _getCurrentState();
        assert(self.owningPlayerIndex >= 0);

        spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};

        const bool left = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "left");
        const bool right = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "right");

        self.x += (10.f * ((float)right - (float)left));

        const bool up = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "up");
        const bool down = wrapper.getSignalValue<bool>(self.owningPlayerIndex, "down");

        self.y += (10.f * ((float)down - (float)up));

        wrapper.pollSimpleEvent(self.owningPlayerIndex, "jump",
                                [&]() {
                                    self.y -= 16.f;
                                });
    }

    void _eventDraw1() override {
        if (isDeactivated()) {
            return;
        }

        const auto& self1 = _getCurrentState();
        const auto& self2 = _getFollowingState();

        const float x = (self1.x + self2.x) * 0.5f;
        const float y = (self1.y + self2.y) * 0.5f;

        sf::CircleShape circle{20.f};
        circle.setFillColor(hg::gr::Color::Green);
        circle.setPosition({x, y});
        ccomp<MWindow>().getCanvas().draw(circle);
    }

    void _syncCreateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncUpdateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncDestroyImpl(spe::SyncDetails& aSyncDetails) const override;
};

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(SinclaireAvatar, (CREATE, UPDATE, DESTROY));

void SinclaireAvatar::_syncCreateImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(SinclaireAvatar, aSyncDetails);
}

void SinclaireAvatar::_syncUpdateImpl(spe::SyncDetails& aSyncDetails) const {
    aSyncDetails.filterSyncs(
        [](hg::PZInteger aRecepient) -> spe::SyncDetails::FilterResult {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                return spe::SyncDetails::FilterResult::Skip;
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                return spe::SyncDetails::FilterResult::Deactivate;
            }
            return spe::SyncDetails::FilterResult::FullSync;
        }
    );
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(SinclaireAvatar, aSyncDetails);
}

void SinclaireAvatar::_syncDestroyImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(SinclaireAvatar, aSyncDetails);
}

///////////////////////////////////////////////////////////////////////////
// MAIN GAMEPLAY CONTROLLER IMPLEMENTATION                               //
///////////////////////////////////////////////////////////////////////////

RN_DEFINE_RPC(SetGlobalStateBufferingLength, RN_ARGS(unsigned, aNewLength)) {
    RN_NODE_IN_HANDLER().callIfClient(
        [=](RN_ClientInterface& aClient) {
            const auto sp = spe::SyncParameters(aClient);
            sp.context.getComponent<MNetworking>().setStateBufferingLength(aNewLength);
            sp.context.getComponent<MInput>().setStateBufferingLength(aNewLength);
            std::cout << "Global state buffering set to " << aNewLength << " frames.\n";
        });
    RN_NODE_IN_HANDLER().callIfServer(
        [](RN_ServerInterface& aClient) {
            throw RN_IllegalMessage();
        });
}

class GameplayManager 
    : public  GameplayManagerInterface
    , public  spe::NonstateObject
    , private spe::NetworkingEventListener {
public:
    GameplayManager(QAO_RuntimeRef aRuntimeRef)
        : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIPRITY_GAMEPLAYMGR, "GameplayManager"}
    {
        ccomp<MNetworking>().addEventListener(*this);
    }

    ~GameplayManager() {
        ccomp<MNetworking>().removeEventListener(*this);
    }

private:
    std::vector<hg::util::StateScheduler<PlayerControls>> _schedulers;

    void _eventUpdate() override;

    void _eventDrawGUI() override {
        sf::Text text;
        text.setFont(
            hg::gr::BuiltInFonts::getFont(
                hg::gr::BuiltInFonts::FontChoice::TitilliumRegular
            )
        );
        std::string s;
        for (int i = 0; i < 5; i += 1) {
            auto& svmMgr = ccomp<spe::SyncedVarmapManagerInterface>();
            auto val = svmMgr.getInt64("val" + std::to_string(i));
            if (val) {
                s += std::to_string(*val) + '\n';
            }
            else {
                s += std::string("n/a") + '\n';
            }
        }
        text.setString(s);
        text.setPosition(40.f, 40.f);

        ccomp<MWindow>().getCanvas().draw(text);
    }

    void _eventFinalizeFrame() override {
        const auto kbInput = ccomp<MWindow>().getKeyboardInput();
        if (kbInput.checkPressed(spe::KbKey::Escape, spe::KbInput::Mode::Direct)) {
            // Stopping the context will delete:
            // - All objects owned by the QAO runtime (in undefined order)
            // - Then, all ContextComponents owned by the context (in reverse order of insertion)
            ctx().stop();
        }
    }

    void onNetworkingEvent(const hg::RN_Event& ev) override {
        if (ccomp<MNetworking>().isClient()) {
            return;
        }

        ev.visit(
            [](const RN_Event::BadPassphrase& ev) {
            },
            [](const RN_Event::ConnectAttemptFailed& ev) {
            },
            [this](const RN_Event::Connected& ev) {
                QAO_PCreate<PlayerAvatar>(getRuntime(),
                                          ccomp<MNetworking>().getRegistryId(),
                                          spe::SYNC_ID_NEW)->init(*ev.clientIndex + 1);
                QAO_PCreate<SinclaireAvatar>(getRuntime(),
                                             ccomp<MNetworking>().getRegistryId(),
                                             spe::SYNC_ID_NEW)->init(*ev.clientIndex + 1);
            },
            [](const RN_Event::Disconnected& ev) {
                // TODO Remove player avatar
                
            }
        );
    }
};

void GameplayManager::_eventUpdate() {
    if (!ctx().isPrivileged() &&
        ccomp<MNetworking>().getClient().getServerConnector().getStatus() == hg::RN_ConnectorStatus::Connected &&
        ccomp<MNetworking>().getLocalPlayerIndex() >= 0) {

        const auto kbInput = ccomp<MWindow>().getKeyboardInput();
        PlayerControls controls{
            kbInput.checkPressed(spe::KbKey::A),
            kbInput.checkPressed(spe::KbKey::D),
            kbInput.checkPressed(spe::KbKey::W),
            kbInput.checkPressed(spe::KbKey::S),
            kbInput.checkPressed(spe::KbKey::Space, spe::KbInput::Mode::Edge)
        };

        spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};
        wrapper.setSignalValue<bool>("left",  controls.left);
        wrapper.setSignalValue<bool>("right", controls.right);
        wrapper.setSignalValue<bool>("up",    controls.up);
        wrapper.setSignalValue<bool>("down",  controls.down);
        wrapper.triggerEvent("jump", controls.jump);

        for (int i = 0; i <= 9; i += 1) {
            if (kbInput.checkPressed(spe::StringToKbKey("Numpad" + std::to_string(i)))) {
                ccomp<spe::SyncedVarmapManagerInterface>().requestToSetInt64(
                    "val" + std::to_string(ccomp<MNetworking>().getLocalPlayerIndex()), 
                    i
                );
            }
        }
    }

    if (ctx().isPrivileged()) {
        const auto kbInput = ccomp<MWindow>().getKeyboardInput();
        const auto mode = spe::KbInput::Mode::Direct;
        if (kbInput.checkPressed(spe::KbKey::Num1, mode)) {
            ctx().getComponent<MNetworking>().setStateBufferingLength(1);
            ctx().getComponent<MInput>().setStateBufferingLength(1);
            Compose_SetGlobalStateBufferingLength(ctx().getComponent<MNetworking>().getNode(), RN_COMPOSE_FOR_ALL, 1);
            std::cout << "Global state buffering set to " << 1 << " frames.\n";
        }
        if (kbInput.checkPressed(spe::KbKey::Num2, mode)) {
            ctx().getComponent<MNetworking>().setStateBufferingLength(2);
            ctx().getComponent<MInput>().setStateBufferingLength(2);
            Compose_SetGlobalStateBufferingLength(ctx().getComponent<MNetworking>().getNode(), RN_COMPOSE_FOR_ALL, 2);
            std::cout << "Global state buffering set to " << 2 << " frames.\n";
        }
        if (kbInput.checkPressed(spe::KbKey::Num3, mode)) {
            ctx().getComponent<MNetworking>().setStateBufferingLength(3);
            ctx().getComponent<MInput>().setStateBufferingLength(3);
            Compose_SetGlobalStateBufferingLength(ctx().getComponent<MNetworking>().getNode(), RN_COMPOSE_FOR_ALL, 3);
            std::cout << "Global state buffering set to " << 3 << " frames.\n";
        }
        if (kbInput.checkPressed(spe::KbKey::Num9, mode)) {
            ctx().getComponent<MNetworking>().setStateBufferingLength(9);
            ctx().getComponent<MInput>().setStateBufferingLength(9);
            Compose_SetGlobalStateBufferingLength(ctx().getComponent<MNetworking>().getNode(), RN_COMPOSE_FOR_ALL, 9);
            std::cout << "Global state buffering set to " << 9 << " frames.\n";
        }
        if (kbInput.checkPressed(spe::KbKey::Num0, mode)) {
            ctx().getComponent<MNetworking>().setStateBufferingLength(0);
            ctx().getComponent<MInput>().setStateBufferingLength(0);
            Compose_SetGlobalStateBufferingLength(ctx().getComponent<MNetworking>().getNode(), RN_COMPOSE_FOR_ALL, 0);
            std::cout << "Global state buffering set to " << 0 << " frames.\n";
        }
    }
}

///////////////////////////////////////////////////////////////////////////
// GAME CONFIG                                                           //
///////////////////////////////////////////////////////////////////////////

#define WINDOW_WIDTH           800
#define WINDOW_HEIGHT          800
#define FRAMERATE               30

bool MyRetransmitPredicate(hg::PZInteger aCyclesSinceLastTransmit,
                           std::chrono::microseconds aTimeSinceLastSend,
                           std::chrono::microseconds aCurrentLatency) {
    // Default behaviour:
    return RN_DefaultRetransmitPredicate(aCyclesSinceLastTransmit,
                                         aTimeSinceLastSend, 
                                         aCurrentLatency);
    // Aggressive retransmission:
    // return 1;
}

enum class GameMode {
    Server, Client
};

std::unique_ptr<spe::GameContext> MakeGameContext(GameMode aGameMode,
                                                  std::uint16_t aLocalPort,
                                                  std::uint16_t aRemotePort, 
                                                  std::string aRemoteIp,
                                                  hg::PZInteger aPlayerCount)
{
    auto context = std::make_unique<spe::GameContext>(
        spe::GameContext::RuntimeConfig{std::chrono::duration<double>(1.0 / FRAMERATE)});
    context->setToMode((aGameMode == GameMode::Server) ? spe::GameContext::Mode::Server
                                                       : spe::GameContext::Mode::Client);

    // Create and attach a Window manager
    auto winMgr = std::make_unique<spe::WindowManagerOne>(context->getQAORuntime().nonOwning(), 
                                                          PRIORITY_WINDOWMGR);
    if (aGameMode == GameMode::Server) {
        winMgr->setToHeadlessMode(spe::WindowManagerInterface::TimingConfig{FRAMERATE});
    }
    else {
        winMgr->setToNormalMode(
            spe::WindowManagerInterface::WindowConfig{
                sf::VideoMode{WINDOW_WIDTH, WINDOW_WIDTH},
                "SPeMPE Minimal Multiplayer",
                sf::Style::Default
            },
            spe::WindowManagerInterface::MainRenderTextureConfig{{WINDOW_HEIGHT, WINDOW_HEIGHT}},
            spe::WindowManagerInterface::TimingConfig{
                FRAMERATE,
                false,                                           /* Framerate limiter */
                (aGameMode == GameMode::Server) ? false : true , /* V-Sync */
                (aGameMode == GameMode::Server) ? true : true    /* Precise timing*/
            }
        );
        winMgr->setMainRenderTextureDrawPosition(spe::WindowManagerInterface::DrawPosition::Stretch);
    }

    context->attachAndOwnComponent(std::move(winMgr));

    // Create and attach a Networking manager
    auto netMgr = std::make_unique<spe::NetworkingManagerOne>(context->getQAORuntime().nonOwning(), 
                                                              PRIORITY_NETWORKMGR,
                                                              STATE_BUFFERING_LENGTH);
    if (aGameMode == GameMode::Server) {
        netMgr->setToMode(spe::NetworkingManagerOne::Mode::Server);
        auto& server = netMgr->getServer();
        server.setTimeoutLimit(std::chrono::seconds{5});
        server.setRetransmitPredicate(&MyRetransmitPredicate);
        server.start(aLocalPort);
        server.resize(aPlayerCount);

        std::printf("Server started on port %d for up to %d clients.\n", (int)server.getLocalPort(), aPlayerCount);
    }
    else {
        netMgr->setToMode(spe::NetworkingManagerOne::Mode::Client);
        auto& client = netMgr->getClient();
        client.setTimeoutLimit(std::chrono::seconds{5});
        client.setRetransmitPredicate(&MyRetransmitPredicate);
        client.connect(aLocalPort, aRemoteIp, aRemotePort);

        std::printf("Client started on port %d (connecting to %s:%d)\n",
                    (int)client.getLocalPort(), aRemoteIp.c_str(), (int)aRemotePort);
    }
    context->attachAndOwnComponent(std::move(netMgr));

    // Create and attack an Input sync manager
    auto insMgr = std::make_unique<spe::InputSyncManagerOne>(context->getQAORuntime().nonOwning(),
                                                             PRIORITY_INPUTMGR);

    if (aGameMode == GameMode::Server) {
        insMgr->setToHostMode(aPlayerCount, STATE_BUFFERING_LENGTH);
    }
    else {
        insMgr->setToClientMode();
    }

    /* Either way, define the inputs in the same way */
    {
        spe::InputSyncManagerWrapper wrapper{*insMgr};
        wrapper.defineSignal<bool>("left", false);
        wrapper.defineSignal<bool>("right", false);
        wrapper.defineSignal<bool>("up", false);
        wrapper.defineSignal<bool>("down", false);
        wrapper.defineSimpleEvent("jump");
    }

    context->attachAndOwnComponent(std::move(insMgr));

    // Create and attach a varmap manager
    auto svmMgr = std::make_unique<spe::DefaultSyncedVarmapManager>(context->getQAORuntime().nonOwning(),
                                                                    PRIORITY_VARMAPMGR);
    if (aGameMode == GameMode::Server) {
        svmMgr->setToMode(spe::SyncedVarmapManagerInterface::Mode::Host);
        for (hg::PZInteger i = 0; i < aPlayerCount; i += 1) {
            svmMgr->int64SetClientWritePermission("val" + std::to_string(i), i, true);
        }
    }
    else {
        svmMgr->setToMode(spe::SyncedVarmapManagerInterface::Mode::Client);
    }

    context->attachAndOwnComponent(std::move(svmMgr));

    // Create and attach a Gameplay manager
    auto gpMgr = std::make_unique<GameplayManager>(context->getQAORuntime().nonOwning());
    context->attachAndOwnComponent(std::move(gpMgr));

    return context;
}

/* SERVER:
 *   mmp.exe server <local-port> <player-count>
 * 
 * CLIENT:
 *   mmp.exe client <local-port> <server-ip> <server-port>
 *
 */
int main(int argc, char* argv[]) {
    RN_IndexHandlers();

    // Parse command line arguments:
    GameMode gameMode;
    std::uint16_t localPort = 0;
    std::uint16_t remotePort = 0;
    hg::PZInteger playerCount = 1;
    std::string remoteIp = "";

    if (argc != 4 && argc != 5) {
        std::puts("Invalid argument count");
        return EXIT_FAILURE;
    }
    const std::string gameModeStr = argv[1];
    if (gameModeStr == "server") {
        gameMode    = GameMode::Server;
        localPort   = std::stoi(argv[2]);
        playerCount = std::stoi(argv[3]);
    }
    else if (gameModeStr == "client") {
        gameMode   = GameMode::Client;
        localPort  = std::stoi(argv[2]);
        remoteIp   = argv[3];
        remotePort = std::stoi(argv[4]);
    }
    else {
        std::puts("Game mode must be either 'server' or 'client'");
        return EXIT_FAILURE;
    }

    if (!(playerCount >= 1 && playerCount < 20)) {
        std::puts("Player count must be between 1 and 20");
        return EXIT_FAILURE;
    }

    // Start the game:
    auto context = MakeGameContext(gameMode, localPort, remotePort,
                                   std::move(remoteIp), playerCount);
    const int status = context->runFor(-1);
    return status;
}

#if 0
int main() {
    float x = 400.f, y = 400.f;

    sf::RenderWindow window(sf::VideoMode(800, 800), "SFML works!");
    //window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    sf::CircleShape shape(30.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        x += (5.f) * ((int)sf::Keyboard::isKeyPressed(sf::Keyboard::D) - 
                      (int)sf::Keyboard::isKeyPressed(sf::Keyboard::A));

        y += (5.f) * ((int)sf::Keyboard::isKeyPressed(sf::Keyboard::S) - 
                      (int)sf::Keyboard::isKeyPressed(sf::Keyboard::W));

        shape.setPosition(x, y);
        window.draw(shape);

        window.display();
    }

    return 0;
}
#endif
