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
#include "Pearl.hpp"
#include "Player_controls.hpp"
#include "Shark.hpp"
#include "Sponge.hpp"
#include "Varmap_ids.hpp"

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Randomization.hpp>

#include <cmath>
#include <optional>
#include <sstream>
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

    auto& varmap = ccomp<spe::SyncedVarmapManagerInterface>();
    for (int i = 0; i < aPlayerCount; i += 1) {
        varmap.stringSetClientWritePermission(VARMAP_ID_PLAYER_STATUS + std::to_string(i), i, true);
    }

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

void MainGameplayManager::depositPearl() {
    auto& varmap    = ccomp<spe::SyncedVarmapManagerInterface>();
    auto  collected = varmap.getInt64(VARMAP_ID_PEARLS_COLLECTED).value();
    ++collected;
    varmap.setInt64(VARMAP_ID_PEARLS_COLLECTED, collected);
}

// MARK: Private

void MainGameplayManager::_startGame(hg::PZInteger aPlayerCount) {
    HG_LOG_INFO(LOG_ID, "Function call: _startGame({})", aPlayerCount);

    if (_gameStageController == nullptr) {
        _gameStageController = QAO_PCreate<GameStageController>(ctx().getQAORuntime(),
                                                                ccomp<MNetworking>().getRegistryId(),
                                                                spe::SYNC_ID_NEW);
    }
    _gameStageController->init();

    auto& varmap = ccomp<MVarmap>();
    varmap.setInt64(VARMAP_ID_PEARLS_REQUIRED, 10);
    varmap.setInt64(VARMAP_ID_PEARLS_COLLECTED, 0);

    auto& lobbyMgr = ccomp<spe::LobbyBackendManagerInterface>();

    const auto sharkPlayerIdx = SelectRandomPlayer(lobbyMgr);
    // auto sharkPlayerIdx = 123;
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

    auto* obj = QAO_PCreate<Sponge>(ctx().getQAORuntime(),
                                    ccomp<MNetworking>().getRegistryId(),
                                    spe::SYNC_ID_NEW);
    obj->init(-50.f, 200.f);

    auto* pearl = QAO_PCreate<Pearl>(ctx().getQAORuntime(),
                                     ccomp<MNetworking>().getRegistryId(),
                                     spe::SYNC_ID_NEW);
    pearl->init(0.f, 200.f);

    ctx().getGameState().isPaused = false;
}

void MainGameplayManager::_restartGame() {
    auto& runtime = *getRuntime();

    std::vector<QAO_Base*> objectsToDelete;
    for (auto* object : runtime) {
        // clang-format off
        if (object->getTypeInfo() == typeid(Diver) ||
            object->getTypeInfo() == typeid(Shark) ||
            object->getTypeInfo() == typeid(LootObject) ||
            object->getTypeInfo() == typeid(Bubble) ||
            object->getTypeInfo() == typeid(Sponge) ||
            object->getTypeInfo() == typeid(GameStageController) ||
            object->getTypeInfo() == typeid(Pearl))
        // clang-format on
        {
            objectsToDelete.push_back(object);
        }
    }
    for (auto* object : objectsToDelete) {
        // if (runtime.ownsObject(object)) {
        //     runtime.eraseObject(object);
        // }
        QAO_PDestroy(object);
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
        auto& varmap = ccomp<spe::SyncedVarmapManagerInterface>();
        if (_gameStageController) {
            if (_gameStageController->getCurrentGameStage() != GAME_STAGE_FINISHED) {
                bool  allDiversDead = true;
                auto& runtime       = *getRuntime();
                for (const QAO_Base* object : runtime) {
                    if (object->getTypeInfo() == typeid(Diver)) {
                        auto* diver = static_cast<const Diver*>(object);
                        if (diver->isAlive()) {
                            allDiversDead = false;
                            break;
                        }
                    }
                }

                if (allDiversDead) {
                    _gameStageController->setCurrentGameStage(GAME_STAGE_FINISHED);
                    varmap.setString(VARMAP_ID_GAMEOVER_STATUS, "The Kraken wins!");
                }

                if (!allDiversDead) {
                    if (*varmap.getInt64(VARMAP_ID_PEARLS_COLLECTED) >=
                        varmap.getInt64(VARMAP_ID_PEARLS_REQUIRED)) {
                        _gameStageController->setCurrentGameStage(GAME_STAGE_FINISHED);
                        varmap.setString(VARMAP_ID_GAMEOVER_STATUS, "The Divers win!");
                    }
                }
            } else {
                // Restart game if anyone pressed Enter
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
        // if (_gameStageController == nullptr) {
        _gameStageController =
            static_cast<GameStageController*>(getRuntime()->find("GameStageController"));
        //}

        const auto input  = ccomp<MWindow>().getInput();
        auto&      client = ccomp<MNetworking>().getClient();
        // If connected, upload input

        if (client.getServerConnector().getStatus() == RN_ConnectorStatus::Connected) {
            const bool left  = input.checkPressed(hg::in::PK_A);
            const bool right = input.checkPressed(hg::in::PK_D);
            const bool up    = input.checkPressed(hg::in::PK_W);
            const bool down  = input.checkPressed(hg::in::PK_S);
            const bool hold  = input.checkPressed(hg::in::PK_SPACE);
            const bool start =
                input.checkPressed(hg::in::PK_ENTER, spe::WindowFrameInputView::Mode::Edge);

            spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};
            wrapper.setSignalValue<bool>(CTRL_ID_LEFT, left);
            wrapper.setSignalValue<bool>(CTRL_ID_RIGHT, right);
            wrapper.setSignalValue<bool>(CTRL_ID_UP, up);
            wrapper.setSignalValue<bool>(CTRL_ID_DOWN, down);
            wrapper.setSignalValue<bool>(CTRL_ID_HOLD, hold);
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

    // Draw game status
    const auto gameStage = getCurrentGameStage();
    if (gameStage != GAME_STAGE_FINISHED) {
        const auto& lobbyMgr = ccomp<MLobbyBackend>();

        std::stringstream oss;
        for (int i = 1; i < lobbyMgr.getSize(); i += 1) {
            const auto& info = lobbyMgr.getLockedInPlayerInfo(i);
            if (info.isEmpty()) {
                continue;
            }
            oss << fmt::format(
                "{} {}\n",
                info.name,
                varmap.getString(VARMAP_ID_PLAYER_STATUS + std::to_string(i)).value_or(""));
        }
        const auto pc = varmap.getInt64(VARMAP_ID_PEARLS_COLLECTED);
        const auto pr = varmap.getInt64(VARMAP_ID_PEARLS_REQUIRED);
        if (pc && pr) {
            oss << "Pearls: " << *pc << " / " << *pr << '\n';
        }

        hg::gr::Text text{hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TITILLIUM_REGULAR),
                          oss.str(),
                          24};
        text.setFillColor(hg::gr::COLOR_WHITE);
        text.setOutlineColor(hg::gr::COLOR_BLACK);
        text.setOutlineThickness(1.f);
        text.setPosition(32.f, 32.f);
        canvas.draw(text);
    } else {
        hg::gr::Text text{hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TITILLIUM_REGULAR),
                          fmt::format("Game Over! {} Press ENTER to play again.",
                                      varmap.getString(VARMAP_ID_GAMEOVER_STATUS).value_or(""))};
        text.setFillColor(hg::gr::COLOR_WHITE);
        text.setOutlineColor(hg::gr::COLOR_BLACK);
        text.setOutlineThickness(2.f);
        text.setPosition(32.f, 32.f);
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
