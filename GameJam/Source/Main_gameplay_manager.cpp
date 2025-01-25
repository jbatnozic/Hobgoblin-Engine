#include "Main_gameplay_manager.hpp"

#include "Bubble.hpp"
#include "Config.hpp"
#include "Diver.hpp"
#include "Environment_manager_interface.hpp"
#include "Game_stage.hpp"
#include "Game_stage_controller.hpp"
#include "Host_menu_manager.hpp"
#include "Join_menu_manager.hpp"
#include "Lobby_frontend_manager_interface.hpp"
#include "Loot.hpp"
#include "Main_menu_manager.hpp"
#include "Player_controls.hpp"
#include "Shark.hpp"
#include "Varmap_ids.hpp"

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Randomization.hpp>

#include <cmath>
#include <optional>
#include <vector>

// MARK: Announcements

namespace {
using hg::gr::BuiltInFonts;
class Announcement : public hg::gr::Drawable {
public:
    Announcement(hg::math::Vector2f aPosition, hg::gr::Color aColor, const std::string& aString)
        : _text{BuiltInFonts::getFont(BuiltInFonts::TITILLIUM_REGULAR), aString} {
        _text.setStyle(hg::gr::Text::BOLD);
        _text.setFillColor(aColor);
        _text.setOutlineColor(hg::gr::COLOR_BLACK);
        _text.setOutlineThickness(2.f);

        const auto bounds = _text.getLocalBounds();
        _text.setOrigin(bounds.getLeft() + bounds.w / 2.0f, bounds.getTop() + bounds.h / 2.0f);
        _text.setPosition(aPosition);
        _text.setScale({0.f, 0.f});
    }

    void update() {
        static constexpr auto  MAX_ANGLE        = hg::math::AngleF::fromDegrees(135.f);
        static constexpr auto  ANGLE_INCREMENT  = MAX_ANGLE / 25.f;
        static constexpr float FADE_INCREMENT   = 1.f / 180.f;
        static constexpr float SCALE_MULTIPLIER = 2.5f;

        if (_angle < MAX_ANGLE) {
            _angle += ANGLE_INCREMENT;
        } else if (_fade < 1.f) {
            _fade += FADE_INCREMENT;
        }

        _text.setScale({_angle.sin() * SCALE_MULTIPLIER, _angle.sin() * SCALE_MULTIPLIER});
        _text.setFillColor(
            _text.getFillColor().withAlpha(static_cast<std::uint8_t>((1.f - _fade) * 255)));
        _text.setOutlineColor(
            _text.getOutlineColor().withAlpha(static_cast<std::uint8_t>((1.f - _fade) * 255)));
    }

    float getFade() const {
        return _fade;
    }

    void move(hg::math::Vector2f aDelta) {
        _text.move(aDelta);
    }

    BatchingType getBatchingType() const override {
        return BatchingType::Custom;
    }

private:
    hg::gr::Text _text;

    hg::math::AngleF _angle = hg::math::AngleF::zero();

    float _fade = 0.f;

    void _drawOnto(hg::gr::Canvas& aCanvas, const hg::gr::RenderStates& aStates) const override {
        aCanvas.draw(_text);
    }
};
} // namespace

class MainGameplayManager::Announcements {
public:
    void add(const hg::gr::Canvas& aCanvas, hg::gr::Color aColor, const std::string& aString) {
        for (auto& item : _items) {
            item.move({0.f, -64.f});
        }
        _items.emplace_back(hg::math::Vector2f{aCanvas.getSize().x * 0.5f, aCanvas.getSize().y * 0.7f},
                            aColor,
                            aString);
    }

    void update() {
        for (auto& item : _items) {
            item.update();
        }

        std::erase_if(_items, [](const Announcement& aItem) {
            return (aItem.getFade() >= 1.f);
        });
    }

    void draw(hg::gr::Canvas& aCanvas) {
        for (auto& item : _items) {
            aCanvas.draw(item);
        }
    }

private:
    std::vector<Announcement> _items;
};

// MARK: Utility

namespace {
std::optional<hg::PZInteger> SelectRandomPlayer(const spe::LobbyBackendManagerInterface& aLobbyMgr) //
{
    std::vector<hg::PZInteger> indices;
    for (hg::PZInteger i = 1; i < aLobbyMgr.getSize(); i += 1) {
        if (aLobbyMgr.getLockedInPlayerInfo(i).isEmpty()) {
            continue;
        }
        indices.push_back(i);
    }
    if (indices.empty()) {
        return std::nullopt;
    }
    hg::util::DoWith64bitRNG([&indices](auto& aRng) {
        std::shuffle(indices.begin(), indices.end(), aRng);
    });
    return indices.front();
}
} // namespace

// MARK: RPC

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

// MARK: MainGameplayManager

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

    _mode        = Mode::HOST;
    _playerCount = aPlayerCount;

    _playerPositions.resize(hg::pztos(aPlayerCount));

    ctx().getGameState().isPaused = true;
}

void MainGameplayManager::setToClientMode() {
    HG_VALIDATE_PRECONDITION(_mode == Mode::UNINITIALIZED);
    _mode = Mode::CLIENT;

    auto& views = ccomp<MWindow>().getViewController();
    views.setViewCount(1);
    views.getView(0).setSize({1920.f / 2.f, 1080.f / 2.f});
    views.getView(0).setViewport({0.f, 0.f, 1.f, 1.f});
    views.getView(0).setCenter({0.f, 0.f});
    views.getView(0).setEnabled(true);

    _announcements = std::make_unique<Announcements>();
}

void MainGameplayManager::startGame() {
    _startGame(_playerCount);
}

MainGameplayManager::Mode MainGameplayManager::getMode() const {
    return _mode;
}

void MainGameplayManager::addAnnouncement(const std::string& aString, hg::gr::Color aColor) {
    _pendingAnnouncements.push_back({aString, aColor});
}

int MainGameplayManager::getCurrentGameStage() const {
    if (_gameStageController == nullptr) {
        return GAME_STAGE_UNKNOWN;
    }
    return _gameStageController->getCurrentGameStage();
}

std::optional<cpVect> MainGameplayManager::getPositionOfClient(int aClientIndex) const {
    if (aClientIndex < 0 || aClientIndex >= (int)_playerPositions.size()) {
        return std::nullopt;
    }
    return _playerPositions[(std::size_t)aClientIndex];
}

void MainGameplayManager::setPositionOfClient(int aClientIndex, cpVect aPosition) {
    HG_VALIDATE_ARGUMENT(aClientIndex >= 0 && aClientIndex < (int)_playerPositions.size());
    _playerPositions[(std::size_t)aClientIndex] = aPosition;
}

void MainGameplayManager::_startGame(hg::PZInteger aPlayerCount) {
    HG_LOG_INFO(LOG_ID, "Function call: _startGame({})", aPlayerCount);

    if (_gameStageController == nullptr) {
        _gameStageController = QAO_PCreate<GameStageController>(ctx().getQAORuntime(),
                                                                ccomp<MNetworking>().getRegistryId(),
                                                                spe::SYNC_ID_NEW);
    }
    _gameStageController->init();

    // auto& varmap   = ccomp<MVarmap>();
    // varmap.setInt64(VARMAP_ID_GAME_STAGE, GAME_STAGE_INITIAL_COUNTDOWN);

    auto& lobbyMgr = ccomp<spe::LobbyBackendManagerInterface>();

    const auto sharkPlayerIdx = SelectRandomPlayer(lobbyMgr);
    for (hg::PZInteger i = 1; i < lobbyMgr.getSize(); i += 1) {
        if (lobbyMgr.getLockedInPlayerInfo(i).isEmpty()) {
            continue;
        }
        if (i == sharkPlayerIdx) {
            auto* obj = QAO_PCreate<Shark>(ctx().getQAORuntime(),
                                           ccomp<MNetworking>().getRegistryId(),
                                           spe::SYNC_ID_NEW);
            obj->init(i, 100.f, 100.f);

            auto* objD = QAO_PCreate<Diver>(ctx().getQAORuntime(),
                                            ccomp<MNetworking>().getRegistryId(),
                                            spe::SYNC_ID_NEW);
            objD->init(0, 200.f, 100.f);
        } else {
            auto* obj = QAO_PCreate<Diver>(ctx().getQAORuntime(),
                                           ccomp<MNetworking>().getRegistryId(),
                                           spe::SYNC_ID_NEW);
            obj->init(i, 100.f, 100.f);
        }
    }

    ctx().getGameState().isPaused = false;
}

void MainGameplayManager::_restartGame() {
    auto& runtime = *getRuntime();

    std::vector<QAO_Base*> objectsToDelete;
    for (auto* object : runtime) {
        if (object->getTypeInfo() == typeid(Diver) || object->getTypeInfo() == typeid(Shark) ||
            object->getTypeInfo() == typeid(LootObject) || object->getTypeInfo() == typeid(Bubble)) {
            objectsToDelete.push_back(object);
        }
    }
    for (auto* object : objectsToDelete) {
        if (runtime.ownsObject(object)) {
            runtime.eraseObject(object);
        }
    }

    // It will have been erased in the loop above
    _gameStageController = nullptr;

    ccomp<MEnvironment>().generatePearls();

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
        childCtx->cleanUp();
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

// MARK: QAO Events

void MainGameplayManager::_eventUpdate1() {
    if (ctx().isPrivileged()) {
#if 0
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
#endif
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
        if (_gameStageController == nullptr) {
            _gameStageController =
                static_cast<GameStageController*>(getRuntime()->find("GameStageController"));
        }

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

        _announcements->update();

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

    for (const auto& announcement : _pendingAnnouncements) {
        _announcements->add(canvas, announcement.color, announcement.string);
    }
    _pendingAnnouncements.clear();

    _announcements->draw(canvas);

#if 0
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
#endif
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
