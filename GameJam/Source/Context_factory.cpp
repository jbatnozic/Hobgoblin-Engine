
#include "Context_factory.hpp"

#include <Hobgoblin/Utility/Randomization.hpp>

#include "Engine.hpp"

#include "Config.hpp"
#include "Environment_manager.hpp"
#include "Lobby_frontend_manager.hpp"
#include "Main_gameplay_manager.hpp"
#include "Player_controls.hpp"
#include "Resource_manager.hpp"

#include "Host_menu_manager.hpp"
#include "Join_menu_manager.hpp"
#include "Main_menu_manager.hpp"

namespace {
constexpr auto FRAME_RATE = 60;
constexpr auto TICK_RATE  = 60;

constexpr auto INITIAL_STATE_BUFFERING_LENGTH = 3;
constexpr auto TELEMETRY_CYCLE_LENGTH         = 300;

constexpr auto PASSPHRASE = "GMTK-2024-HQ";

bool MyRetransmitPredicate(hg::PZInteger             aCyclesSinceLastTransmit,
                           std::chrono::microseconds aTimeSinceLastSend,
                           std::chrono::microseconds aCurrentLatency) {
    // Default behaviour:
    return RN_DefaultRetransmitPredicate(aCyclesSinceLastTransmit, aTimeSinceLastSend, aCurrentLatency);

    // Aggressive retransmission:
    // return true;
}
} // namespace

///////////////////////////////////////////////////////////////////////////
// MARK: SERVER                                                          //
///////////////////////////////////////////////////////////////////////////

std::unique_ptr<spe::GameContext> CreateServerContext(const ServerGameParams& aParams) {
    auto context =
        std::make_unique<spe::GameContext>(spe::GameContext::RuntimeConfig{spe::TickRate{TICK_RATE}});
    context->setToMode(spe::GameContext::Mode::Server);

    // Window manager
    auto winMgr = QAO_UPCreate<spe::DefaultWindowManager>(context->getQAORuntime().nonOwning(),
                                                          PRIORITY_WINDOWMGR);
    spe::WindowManagerInterface::TimingConfig timingConfig{
#ifdef _MSC_VER
        spe::FrameRate{FRAME_RATE},
        spe::PREVENT_BUSY_WAIT_ON,
        spe::VSYNC_ON
#else
        0,
        spe::PREVENT_BUSY_WAIT_OFF,
        spe::VSYNC_OFF
#endif
    };
    winMgr->setToHeadlessMode(timingConfig);
    context->attachAndOwnComponent(std::move(winMgr));

    // Loop timing reporter
    QAO_PCreate<spe::EventLoopTimingReporter>(
        context->getQAORuntime(),
        0,
        spe::EventLoopTimingReporter::Config{TELEMETRY_CYCLE_LENGTH});

    // Networking manager
    auto netMgr = QAO_UPCreate<spe::DefaultNetworkingManager>(context->getQAORuntime().nonOwning(),
                                                              PRIORITY_NETWORKMGR,
                                                              INITIAL_STATE_BUFFERING_LENGTH);

    const auto clientCount = aParams.playerCount - 1; // -1 because player 0 is the host itself
                                                      // (even if it doesn't participate in the game)
    netMgr->setToServerMode(RN_Protocol::UDP,
                            PASSPHRASE,
                            clientCount,
                            2048,
                            aParams.zeroTierEnabled ? RN_NetworkingStack::ZeroTier
                                                    : RN_NetworkingStack::Default);
    netMgr->setPacemakerPulsePeriod(120);
    auto& server = netMgr->getServer();
    server.setTimeoutLimit(std::chrono::seconds{5});
    server.setRetransmitPredicate(&MyRetransmitPredicate);
    server.start(aParams.portNumber);

    HG_LOG_INFO(LOG_ID,
                "Server started on port {} for {} players.",
                server.getLocalPort(),
                aParams.playerCount - 1);

    netMgr->setTelemetryCycleLimit(TELEMETRY_CYCLE_LENGTH);
    context->attachAndOwnComponent(std::move(netMgr));

    // Telemetry reporter
    QAO_PCreate<spe::NetworkingTelemetryReporter>(
        context->getQAORuntime(),
        0,
        spe::NetworkingTelemetryReporter::Config{TELEMETRY_CYCLE_LENGTH});

    // Input sync manager
    auto insMgr = QAO_UPCreate<spe::DefaultInputSyncManager>(context->getQAORuntime().nonOwning(),
                                                             PRIORITY_INPUTMGR);

    insMgr->setToHostMode(clientCount, INITIAL_STATE_BUFFERING_LENGTH);
    SetUpPlayerControls(*insMgr);
    context->attachAndOwnComponent(std::move(insMgr));

    // Varmap manager
    auto svmMgr = QAO_UPCreate<spe::DefaultSyncedVarmapManager>(context->getQAORuntime().nonOwning(),
                                                                PRIORITY_VARMAPMGR);
    svmMgr->setToMode(spe::SyncedVarmapManagerInterface::Mode::Host);
    context->attachAndOwnComponent(std::move(svmMgr));

    // Lobby backend manager
    auto lobbyMgr =
        std::make_unique<spe::DefaultLobbyBackendManager>(context->getQAORuntime().nonOwning(),
                                                          PRIORITY_LOBBYBACKMGR);

    lobbyMgr->setToHostMode(aParams.playerCount);
    context->attachAndOwnComponent(std::move(lobbyMgr));

    // Lobby frontend manager
    auto lobbyFrontendMgr =
        QAO_UPCreate<LobbyFrontendManager>(context->getQAORuntime().nonOwning(), PRIORITY_LOBBYFRONTMGR);

    lobbyFrontendMgr->setToHeadlessHostMode();
    context->attachAndOwnComponent(std::move(lobbyFrontendMgr));

    // Auth manager
    auto authMgr = QAO_UPCreate<spe::DefaultAuthorizationManager>(context->getQAORuntime().nonOwning(),
                                                                  PRIORITY_AUTHMGR);
    authMgr->setToHostMode();
    context->attachAndOwnComponent(std::move(authMgr));

    // Resource manager
    auto resMgr =
        QAO_UPCreate<ResourceManager>(context->getQAORuntime().nonOwning(), PRIORITY_RESOURCEMGR);
    resMgr->setToHostMode();
    context->attachAndOwnComponent(std::move(resMgr));

    // Environment manager
    auto envMgr =
        QAO_UPCreate<EnvironmentManager>(context->getQAORuntime().nonOwning(), PRIORITY_GAMEPLAYMGR);
    auto* envMgrP = envMgr.get();
    envMgr->setToHeadlessHostMode();
    context->attachAndOwnComponent(std::move(envMgr));
    envMgrP->generateTerrain(terrain_size, terrain_size);

    // Main gameplay manager
    auto gpMgr =
        QAO_UPCreate<MainGameplayManager>(context->getQAORuntime().nonOwning(), PRIORITY_GAMEPLAYMGR);
    gpMgr->setToHostMode(aParams.playerCount);
    context->attachAndOwnComponent(std::move(gpMgr));

    return context;
}

///////////////////////////////////////////////////////////////////////////
// MARK: CLIENT - BASIC                                                  //
///////////////////////////////////////////////////////////////////////////

#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 900

// Main Render Texture (MRT) size determines the resolution
// at which the game will be rendered internally.
#define MRT_WIDTH  1920
#define MRT_HEIGHT 1080

std::unique_ptr<spe::GameContext> CreateBasicClientContext() {
    auto context =
        std::make_unique<spe::GameContext>(spe::GameContext::RuntimeConfig{spe::TickRate{TICK_RATE}});
    context->setToMode(spe::GameContext::Mode::Client);

    // Window manager
    auto winMgr = QAO_UPCreate<spe::DefaultWindowManager>(context->getQAORuntime().nonOwning(),
                                                          PRIORITY_WINDOWMGR);
    spe::WindowManagerInterface::TimingConfig timingConfig{
#ifdef _MSC_VER
        spe::FrameRate{FRAME_RATE},
        spe::PREVENT_BUSY_WAIT_ON,
        spe::VSYNC_OFF
#else
        0,
        spe::PREVENT_BUSY_WAIT_OFF,
        spe::VSYNC_OFF
#endif
    };

    // clang-format off
    winMgr->setToNormalMode(
        spe::WindowManagerInterface::WindowConfig{
            hg::win::VideoMode{WINDOW_WIDTH, WINDOW_HEIGHT},
            "BIG QUEST: THE GAME",
            hg::win::WindowStyle::Default
        },
        spe::WindowManagerInterface::MainRenderTextureConfig{{MRT_WIDTH, MRT_HEIGHT}},
        timingConfig);
    // clang-format on
    winMgr->setStopIfCloseClicked(true);

    struct FontFace {
        Rml::String filename;
        bool        fallback_face;
    };
    FontFace font_faces[] = {
        {   "LatoLatin-Regular.ttf", false},
        {    "LatoLatin-Italic.ttf", false},
        {      "LatoLatin-Bold.ttf", false},
        {"LatoLatin-BoldItalic.ttf", false},
    };
    for (const FontFace& face : font_faces) {
        Rml::LoadFontFace("assets/fonts/" + face.filename, face.fallback_face);
    }

#ifndef NDEBUG
    Rml::Debugger::Initialise(&(winMgr->getGUIContext()));
    Rml::Debugger::SetVisible(true);
#endif

    context->attachAndOwnComponent(std::move(winMgr));

    // Loop timing reporter
    QAO_PCreate<spe::EventLoopTimingReporter>(
        context->getQAORuntime(),
        0,
        spe::EventLoopTimingReporter::Config{TELEMETRY_CYCLE_LENGTH});

    // Resource manager
    auto resMgr =
        QAO_UPCreate<ResourceManager>(context->getQAORuntime().nonOwning(), PRIORITY_RESOURCEMGR);
    resMgr->setToClientMode();
    context->attachAndOwnComponent(std::move(resMgr));

    // Main menu manager
    auto mainMenuMgr =
        QAO_UPCreate<MainMenuManager>(context->getQAORuntime().nonOwning(), PRIORITY_MAINMENUMGR);
    context->attachAndOwnComponent(std::move(mainMenuMgr));

    // Host menu manager
    auto hostMenuMgr =
        QAO_UPCreate<HostMenuManager>(context->getQAORuntime().nonOwning(), PRIORITY_HOSTMENUMGR);
    hostMenuMgr->setVisible(false);
    context->attachAndOwnComponent(std::move(hostMenuMgr));

    // Join menu manager
    auto joinMenuMgr =
        QAO_UPCreate<JoinMenuManager>(context->getQAORuntime().nonOwning(), PRIORITY_JOINMENUMGR);
    joinMenuMgr->setVisible(false);
    context->attachAndOwnComponent(std::move(joinMenuMgr));

    return context;
}

///////////////////////////////////////////////////////////////////////////
// MARK: CLIENT - EXTRA                                                  //
///////////////////////////////////////////////////////////////////////////

void AttachGameplayManagers(spe::GameContext& aContext, const ClientGameParams& aParams) {
    // Networking manager
    auto netMgr = QAO_UPCreate<spe::DefaultNetworkingManager>(aContext.getQAORuntime().nonOwning(),
                                                              PRIORITY_NETWORKMGR,
                                                              INITIAL_STATE_BUFFERING_LENGTH);
    netMgr->setToClientMode(RN_Protocol::UDP,
                            PASSPHRASE,
                            2048,
                            aParams.zeroTierEnabled ? RN_NetworkingStack::ZeroTier
                                                    : RN_NetworkingStack::Default);
    auto& client = netMgr->getClient();
    client.setTimeoutLimit(std::chrono::seconds{5});
    client.setRetransmitPredicate(&MyRetransmitPredicate);

    if (!aParams.skipConnect) {
        client.connect(aParams.localPortNumber, aParams.hostIpAddress, aParams.hostPortNumber);
    }

    HG_LOG_INFO(LOG_ID,
                "Client started on port {} (connecting to {}:{}).",
                client.getLocalPort(),
                aParams.hostIpAddress,
                aParams.hostPortNumber);

    netMgr->setTelemetryCycleLimit(TELEMETRY_CYCLE_LENGTH);
    aContext.attachAndOwnComponent(std::move(netMgr));

    // Telemetry reporter
    QAO_PCreate<spe::NetworkingTelemetryReporter>(
        aContext.getQAORuntime(),
        0,
        spe::NetworkingTelemetryReporter::Config{TELEMETRY_CYCLE_LENGTH});

    // Input sync manager
    auto insMgr = QAO_UPCreate<spe::DefaultInputSyncManager>(aContext.getQAORuntime().nonOwning(),
                                                             PRIORITY_INPUTMGR);

    insMgr->setToClientMode();
    SetUpPlayerControls(*insMgr);
    aContext.attachAndOwnComponent(std::move(insMgr));

    // Varmap manager
    auto svmMgr = QAO_UPCreate<spe::DefaultSyncedVarmapManager>(aContext.getQAORuntime().nonOwning(),
                                                                PRIORITY_VARMAPMGR);

    svmMgr->setToMode(spe::SyncedVarmapManagerInterface::Mode::Client);
    aContext.attachAndOwnComponent(std::move(svmMgr));

    // Lobby backend manager
    auto lobbyMgr = QAO_UPCreate<spe::DefaultLobbyBackendManager>(aContext.getQAORuntime().nonOwning(),
                                                                  PRIORITY_LOBBYBACKMGR);

    lobbyMgr->setToClientMode(1);
    aContext.attachAndOwnComponent(std::move(lobbyMgr));

    // Lobby frontend manager
    auto lobbyFrontendMgr =
        QAO_UPCreate<LobbyFrontendManager>(aContext.getQAORuntime().nonOwning(), PRIORITY_LOBBYFRONTMGR);

    const auto uniqueId = "id_" + std::to_string(hg::util::GetRandomNumber<int>(10'000, 99'999));
    lobbyFrontendMgr->setToClientMode(aParams.playerName, uniqueId);
    aContext.attachAndOwnComponent(std::move(lobbyFrontendMgr));

    // Auth manager
    auto authMgr = QAO_UPCreate<spe::DefaultAuthorizationManager>(aContext.getQAORuntime().nonOwning(),
                                                                  PRIORITY_AUTHMGR);

    authMgr->setToClientMode();
    aContext.attachAndOwnComponent(std::move(authMgr));

    // Environment manager
    auto envMgr =
        QAO_UPCreate<EnvironmentManager>(aContext.getQAORuntime().nonOwning(), PRIORITY_GAMEPLAYMGR);
    envMgr->setToClientMode();
    aContext.attachAndOwnComponent(std::move(envMgr));

    // Gameplay manager
    auto gpMgr =
        QAO_UPCreate<MainGameplayManager>(aContext.getQAORuntime().nonOwning(), PRIORITY_GAMEPLAYMGR);
    gpMgr->setToClientMode();
    aContext.attachAndOwnComponent(std::move(gpMgr));
}
