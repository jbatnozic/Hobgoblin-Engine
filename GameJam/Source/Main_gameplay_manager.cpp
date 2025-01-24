#include "Main_gameplay_manager.hpp"

// #include "Character.hpp"
#include "Config.hpp"
#include "Diver.hpp"
#include "Environment_manager_interface.hpp"
#include "Host_menu_manager.hpp"
#include "Join_menu_manager.hpp"
#include "Lobby_frontend_manager_interface.hpp"
#include "Loot.hpp"
#include "Main_menu_manager.hpp"
#include "Player_controls.hpp"
#include "Varmap_ids.hpp"

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>

#include <cmath>

namespace {
constexpr int WIN_TIMER_MAX = 10 * 60;
}

RN_DEFINE_RPC(SetGlobalStateBufferingLength, RN_ARGS(unsigned, aNewLength)) {
    RN_NODE_IN_HANDLER().callIfClient([=](RN_ClientInterface& aClient) {
        const auto rc = spe::RPCReceiverContext(aClient);
        rc.gameContext.getComponent<MNetworking>().setStateBufferingLength(aNewLength);
        rc.gameContext.getComponent<MInput>().setStateBufferingLength(aNewLength);
        HG_LOG_INFO(LOG_ID, "Global state buffering set to {} frames.", aNewLength);
    });
    RN_NODE_IN_HANDLER().callIfServer([](RN_ServerInterface&) {
        throw RN_IllegalMessage();
    });
}

MainGameplayManager::MainGameplayManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "GameplayManager"} {
    auto& netMgr = ccomp<MNetworking>();
    netMgr.addEventListener(this);
    // stateBufferingLength = netMgr.getStateBufferingLength();
}

MainGameplayManager::~MainGameplayManager() {
    ccomp<MNetworking>().removeEventListener(this);
}

void MainGameplayManager::setToHostMode(hg::PZInteger aPlayerCount) {
    HG_VALIDATE_PRECONDITION(_mode == Mode::UNINITIALIZED);
    _mode = Mode::HOST;
    _startGame(aPlayerCount);
}

void MainGameplayManager::setToClientMode() {
    HG_VALIDATE_PRECONDITION(_mode == Mode::UNINITIALIZED);
    _mode = Mode::CLIENT;

    auto& views = ccomp<MWindow>().getViewController();
    views.setViewCount(2);

    views.getView(0).setSize({1920.f, 1080.f});
    views.getView(0).setViewport({0.f, 0.f, 1.f, 1.f});
    views.getView(0).setCenter({0.f, 0.f});
    views.getView(0).zoom(2.0);
    views.getView(0).setEnabled(true);

    views.getView(1).setSize({1920.f, 1080.f});
    views.getView(1).setViewport({0.f, 0.f, 1.f, 1.f});
    views.getView(1).setCenter({0.f, 0.f});
    views.getView(1).zoom(3.0);
    views.getView(1).setEnabled(false);
}

MainGameplayManager::Mode MainGameplayManager::getMode() const {
    return _mode;
}

void MainGameplayManager::characterReachedTheScales(CharacterObject& aCharacter) {
    auto& varmap       = ccomp<MVarmap>();
    auto& lobbyBackend = ccomp<MLobbyBackend>();

    if (contender1 == nullptr) {
        contender1 = &aCharacter;

        varmap.setInt64(VARMAP_ID_WIN_TIMER, WIN_TIMER_MAX);
        varmap.setString(VARMAP_ID_CONTENDER_1_NAME,
                         lobbyBackend.getLockedInPlayerInfo(aCharacter.getOwningPlayerIndex()).name);
    } else if (contender2 == nullptr && contender1 != &aCharacter &&
               *varmap.getInt64(VARMAP_ID_GAME_OVER) == 0) {
        contender2 = &aCharacter;

        varmap.setInt64(VARMAP_ID_WIN_TIMER, -1);
        varmap.setString(VARMAP_ID_CONTENDER_2_NAME,
                         lobbyBackend.getLockedInPlayerInfo(aCharacter.getOwningPlayerIndex()).name);
        varmap.setInt64(VARMAP_ID_GAME_OVER, 1);

        // Determine winner
        if ((std::abs(contender1->getMass() - contender2->getMass()) < 0.01) ||
            (contender1->getMass() > contender2->getMass())) {
            varmap.setString(VARMAP_ID_WINNER_NAME, *varmap.getString(VARMAP_ID_CONTENDER_1_NAME));
        } else if (contender1->getMass() - contender2->getMass()) {
            varmap.setString(VARMAP_ID_WINNER_NAME, *varmap.getString(VARMAP_ID_CONTENDER_2_NAME));
        }
    }
}

void MainGameplayManager::_startGame(hg::PZInteger aPlayerCount) {
    HG_LOG_INFO(LOG_ID, "Function call: _startGame({})", aPlayerCount);

    _playerCount = aPlayerCount;

    auto& varmap = ccomp<MVarmap>();
    varmap.setInt64(VARMAP_ID_WIN_TIMER, -1);
    varmap.setString(VARMAP_ID_CONTENDER_1_NAME, "n/a");
    varmap.setString(VARMAP_ID_CONTENDER_2_NAME, "n/a");
    varmap.setInt64(VARMAP_ID_GAME_OVER, 0);
    varmap.setString(VARMAP_ID_WINNER_NAME, "n/a");

    auto& envMgr = ccomp<MEnvironment>();
    // for (hg::PZInteger i = 0; i < aPlayerCount; i += 1) {
    //     if (i == 0) {
    //         continue; // player 0 is the host, doesn't need a character
    //     }
    //     auto* obj = QAO_PCreate<CharacterObject>(ctx().getQAORuntime(),
    //                                              ccomp<MNetworking>().getRegistryId(),
    //                                              spe::SYNC_ID_NEW);
    //     obj->init(i,
    //               left_offset * single_terrain_size + i * 300.0,
    //               (terrain_size + 5) * single_terrain_size - 300.0);
    // }

    auto* obj = QAO_PCreate<Diver>(ctx().getQAORuntime(),
                                   ccomp<MNetworking>().getRegistryId(),
                                   spe::SYNC_ID_NEW);
    obj->init(1, 1000.f, 1000.f);
}

void MainGameplayManager::_restartGame() {
    auto& runtime = *getRuntime();

    std::vector<QAO_Base*> objectsToDelete;
    for (auto* object : runtime) {
        if (object->getTypeInfo() == typeid(CharacterObject) ||
            object->getTypeInfo() == typeid(LootObject)) {
            objectsToDelete.push_back(object);
        }
    }
    for (auto* object : objectsToDelete) {
        if (runtime.ownsObject(object)) {
            runtime.eraseObject(object);
        }
    }

    ccomp<MEnvironment>().generateLoot();

    contender1 = nullptr;
    contender2 = nullptr;

    _startGame(_playerCount);
}

namespace {
template <class taComponent>
void DetachAndDestroyComponent(spe::GameContext& aCtx) {
    spe::DetachStatus detachStatus;
    auto              mgr = aCtx.detachComponent<taComponent>(&detachStatus);
    HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && mgr != nullptr);
    mgr.reset();
}
} // namespace

void MainGameplayManager::_backToMainMenu() {
    // Kill child (if any)
    if (ctx().hasChildContext() && ctx().isChildContextJoinable()) {
        const auto childStatus = ctx().stopAndJoinChildContext();
        HG_LOG_INFO(LOG_ID, "Child context stopped with exit code {}.", childStatus);
        auto childCtx = ctx().detachChildContext();
        childCtx.reset();
    }

    auto& context = ctx();
    auto& runtime = context.getQAORuntime();
    runtime.destroyAllOwnedObjects();

    DetachAndDestroyComponent<MEnvironment>(context);
    DetachAndDestroyComponent<spe::AuthorizationManagerInterface>(context);
    DetachAndDestroyComponent<LobbyFrontendManagerInterface>(context);
    DetachAndDestroyComponent<MLobbyBackend>(context);
    DetachAndDestroyComponent<MVarmap>(context);
    DetachAndDestroyComponent<spe::InputSyncManagerInterface>(context);
    DetachAndDestroyComponent<MainGameplayManagerInterface>(context); // WARNING: `this` will be deleted!
    DetachAndDestroyComponent<MNetworking>(context);

    context.getComponent<MWindow>().resetGUIContext();

    // Main menu manager
    auto mainMenuMgr = QAO_UPCreate<MainMenuManager>(runtime.nonOwning(), PRIORITY_MAINMENUMGR);
    context.attachAndOwnComponent(std::move(mainMenuMgr));

    // Host menu manager
    auto hostMenuMgr = QAO_UPCreate<HostMenuManager>(runtime.nonOwning(), PRIORITY_HOSTMENUMGR);
    hostMenuMgr->setVisible(false);
    context.attachAndOwnComponent(std::move(hostMenuMgr));

    // Join menu manager
    auto joinMenuMgr = QAO_UPCreate<JoinMenuManager>(runtime.nonOwning(), PRIORITY_JOINMENUMGR);
    joinMenuMgr->setVisible(false);
    context.attachAndOwnComponent(std::move(joinMenuMgr));
}

void MainGameplayManager::_eventUpdate1() {
    if (ctx().isPrivileged()) {
        auto& varmap       = ccomp<MVarmap>();
        auto& lobbyBackend = ccomp<MLobbyBackend>();

        auto winTimer = *varmap.getInt64(VARMAP_ID_WIN_TIMER);
        auto gameOver = *varmap.getInt64(VARMAP_ID_GAME_OVER);
        if (winTimer > 0 && gameOver == 0) {
            winTimer -= 1;
            if (winTimer > 0) {
                varmap.setInt64(VARMAP_ID_WIN_TIMER, winTimer);
            } else {
                // Contender 1 wins
                HG_HARD_ASSERT(contender1 != nullptr);
                HG_HARD_ASSERT(contender2 == nullptr);

                varmap.setInt64(VARMAP_ID_WIN_TIMER, -1);
                varmap.setString(
                    VARMAP_ID_WINNER_NAME,
                    lobbyBackend.getLockedInPlayerInfo(contender1->getOwningPlayerIndex()).name);
                varmap.setInt64(VARMAP_ID_GAME_OVER, 1);
            }
        }

        // Restart game if anyone pressed Enter
        if (gameOver == 1) {
            auto&                        netMgr = ccomp<MNetworking>();
            spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};

            bool startPressed = false;
            for (hg::PZInteger i = 0; i < netMgr.getServer().getSize() && !startPressed; i += 1) {
                wrapper.pollSimpleEvent(i, CTRL_ID_START, [&]() {
                    startPressed = true;
                });
            }
            if (startPressed) {
                _restartGame();
            }
        }
#if 0
        auto& winMgr = ccomp<MWindow>();

        const int MAX_BUFFERING_LENGTH = 10;
        bool sync = false;

        if (winMgr.getInput().checkPressed(hg::in::PK_I,
                                           spe::WindowFrameInputView::Mode::Direct)) {
            stateBufferingLength = (stateBufferingLength + 1) % (MAX_BUFFERING_LENGTH + 1);
            sync = true;
        }
        if (winMgr.getInput().checkPressed(hg::in::PK_U,
                                           spe::WindowFrameInputView::Mode::Direct)) {
            stateBufferingLength = (stateBufferingLength + MAX_BUFFERING_LENGTH) % (MAX_BUFFERING_LENGTH + 1);
            sync = true;
        }

        if (sync) {
            HG_LOG_INFO(LOG_ID, "Global state buffering set to {} frames.", stateBufferingLength);
            Compose_SetGlobalStateBufferingLength(
                ccomp<MNetworking>().getNode(),
                RN_COMPOSE_FOR_ALL,
                stateBufferingLength
            );
        }

        return;
#endif
    }

    if (!ctx().isPrivileged()) {
        const auto input  = ccomp<MWindow>().getInput();
        auto&      client = ccomp<MNetworking>().getClient();
        // If connected, upload input

        if (client.getServerConnector().getStatus() == RN_ConnectorStatus::Connected) {
            const bool left  = input.checkPressed(hg::in::PK_A);
            const bool right = input.checkPressed(hg::in::PK_D);
            const bool up    = input.checkPressed(hg::in::PK_W);
            const bool down  = input.checkPressed(hg::in::PK_S);
            const bool jump =
                input.checkPressed(hg::in::PK_SPACE, spe::WindowFrameInputView::Mode::Edge);
            const bool start =
                input.checkPressed(hg::in::PK_ENTER, spe::WindowFrameInputView::Mode::Edge);

            spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};
            wrapper.setSignalValue<bool>(CTRL_ID_LEFT, left);
            wrapper.setSignalValue<bool>(CTRL_ID_RIGHT, right);
            wrapper.setSignalValue<bool>(CTRL_ID_UP, up);
            wrapper.setSignalValue<bool>(CTRL_ID_DOWN, down);
            wrapper.triggerEvent(CTRL_ID_JUMP, jump);
            wrapper.triggerEvent(CTRL_ID_START, start);
        }

        if (input.checkPressed(hg::in::PK_ESCAPE, spe::WindowFrameInputView::Mode::Edge)) {
            _backToMainMenu(); // WARNING: this will delete `this`!
        }
    }
}

void MainGameplayManager::_eventDrawGUI() {
    using namespace hg::gr;

    auto& winMgr = ccomp<MWindow>();
    auto& varmap = ccomp<MVarmap>();
    auto& canvas = winMgr.getCanvas();

    const auto winTimer       = varmap.getInt64(VARMAP_ID_WIN_TIMER);
    const auto contender1name = varmap.getString(VARMAP_ID_CONTENDER_1_NAME);
    const auto gameOver       = varmap.getInt64(VARMAP_ID_GAME_OVER);
    const auto winnerName     = varmap.getString(VARMAP_ID_WINNER_NAME);

    Text text{BuiltInFonts::getFont(BuiltInFonts::FontChoice::TITILLIUM_REGULAR)};
    text.setCharacterSize(30);
    text.setStyle(Text::BOLD);
    text.setFillColor(COLOR_WHITE);
    text.setOutlineColor(COLOR_BLACK);
    text.setOutlineThickness(4.f);

    bool doDraw = true;
    if (gameOver.has_value() && *gameOver == 1) {
        text.setString(fmt::format(FMT_STRING("{} wins! Press ENTER to play again."),
                                   (winnerName.has_value() ? *winnerName : "A player")));
    } else if (winTimer.has_value() && *winTimer > 0) {
        text.setString(fmt::format(
            FMT_STRING("{} has reached the top! Hurry up!\n               {} seconds remaining!"),
            (contender1name.has_value() ? *contender1name : "A player"),
            *winTimer / 60));
    } else {
        doDraw = false;
    }

    if (doDraw) {
        // const auto& bounds = text.getLocalBounds();
        // text.setOrigin({bounds.x / 2.f, bounds.y / 2.f});
        // text.setPosition({winMgr.getWindowSize().x / 2.f, 100.f});
        text.setPosition({100.f, 100.f});
        canvas.draw(text);
    }
}

void MainGameplayManager::_eventPostUpdate() {
    const auto input = ccomp<MWindow>().getInput();
    if (input.checkPressed(hg::in::PK_F9, spe::WindowFrameInputView::Mode::Direct)) {
        // Stopping the context will delete:
        // - All objects owned by the QAO runtime (in undefined order)
        // - Then, all ContextComponents owned by the context (in reverse order of insertion)
        ctx().stop();
    }
}

void MainGameplayManager::onNetworkingEvent(const hg::RN_Event& aEvent) {
    if (ccomp<MNetworking>().isClient()) {
        // CLIENT
        aEvent.visit([this](const RN_Event::Connected& ev) {
            HG_LOG_INFO(LOG_ID, "Client lobby uploading local info to server.");
            ccomp<MLobbyBackend>().uploadLocalInfo();
        });
    } else {
        // HOST
        aEvent.visit([this](const RN_Event::Connected& ev) {},
                     [](const RN_Event::Disconnected& ev) {
                         // TODO Remove player avatar
                     });
    }
}
