#include "Managers/Lobby_frontend_manager.hpp"

#include "Managers/Main_gameplay_manager_interface.hpp"

#include "Engine.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/RigelNet_macros.hpp>
#include <Hobgoblin/Utility/No_copy_no_move.hpp>
#include <RmlUi/Core.h>

namespace {
template <class T>
bool RegisterModel(Rml::DataModelConstructor& aDataModelCtor) {
    return false;
}

struct PlayerInfoModel {
    Rml::String name;
    Rml::String uuid;
    Rml::String ip;
};

template <>
bool RegisterModel<PlayerInfoModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<PlayerInfoModel>();
    if (handle) {
        handle.RegisterMember("name", &PlayerInfoModel::name);
        handle.RegisterMember("uuid", &PlayerInfoModel::uuid);
        handle.RegisterMember("ip", &PlayerInfoModel::ip);
    }
    return static_cast<bool>(handle);
}

struct DualPlayerInfoModel {
    PlayerInfoModel lockedIn;
    PlayerInfoModel pending;
    bool            showPending = false;
};

template <>
bool RegisterModel<DualPlayerInfoModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<DualPlayerInfoModel>();
    if (handle) {
        handle.RegisterMember("lockedIn", &DualPlayerInfoModel::lockedIn);
        handle.RegisterMember("pending", &DualPlayerInfoModel::pending);
        handle.RegisterMember("showPending", &DualPlayerInfoModel::showPending);
    }
    return static_cast<bool>(handle);
}

struct LobbyModel {
    std::vector<DualPlayerInfoModel> players;
    Rml::String                      localName;
    bool                             isAuthorized = false;
    // Rml::String                      localIpAddress = "n/a";
};

template <>
bool RegisterModel<LobbyModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<LobbyModel>();
    if (handle) {
        handle.RegisterMember("players", &LobbyModel::players);
    }
    return static_cast<bool>(handle);
}
} // namespace

#define COMMAND_LOCK_IN 0
#define COMMAND_RESET   1
#define COMMAND_MOVE_UP 2
#define COMMAND_MOVE_DN 3
#define COMMAND_KICK    4
#define COMMAND_START   5

void ActivateCommand(LobbyFrontendManager& aMgr, int aCommand, void* aArgs) {
    auto& lobbyBackendMgr = aMgr.ccomp<MLobbyBackend>();
    switch (aCommand) {
    case COMMAND_LOCK_IN:
        HG_LOG_INFO(LOG_ID, "Locking in lobby.");
        lobbyBackendMgr.lockInPendingChanges();
        break;

    case COMMAND_RESET:
        HG_LOG_INFO(LOG_ID, "Resetting lobby.");
        lobbyBackendMgr.resetPendingChanges();
        break;

    case COMMAND_MOVE_UP:
        {
            const auto size       = lobbyBackendMgr.getSize();
            auto       slotIndex1 = *static_cast<hg::PZInteger*>(aArgs);
            if (slotIndex1 == 0)
                break;
            auto slotIndex2 = (slotIndex1 + size - 1) % size;
            if (slotIndex2 == 0)
                slotIndex2 = (slotIndex1 + size - 2) % size;
            if (slotIndex2 == 0)
                break;

            HG_LOG_INFO(LOG_ID, "Swapping slots {} and {}.", slotIndex1, slotIndex2);
            lobbyBackendMgr.beginSwap(slotIndex1, slotIndex2);
        }
        break;

    case COMMAND_MOVE_DN:
        {
            const auto size       = lobbyBackendMgr.getSize();
            auto       slotIndex1 = *static_cast<hg::PZInteger*>(aArgs);
            if (slotIndex1 == 0)
                break;
            auto slotIndex2 = (slotIndex1 + 1) % size;
            if (slotIndex2 == 0)
                slotIndex2 = (slotIndex1 + 2) % size;
            if (slotIndex2 == 0)
                break;

            HG_LOG_INFO(LOG_ID, "Swapping slots {} and {}.", slotIndex1, slotIndex2);
            lobbyBackendMgr.beginSwap(slotIndex1, slotIndex2);
        }
        break;

    case COMMAND_KICK:
        {
            const auto slotIndex   = *static_cast<hg::PZInteger*>(aArgs);
            const auto clientIndex = lobbyBackendMgr.playerIdxToClientIdx(slotIndex);

            if (clientIndex >= 0) {
                auto& netMgr = aMgr.ccomp<MNetworking>();
                netMgr.getServer().kickClient(clientIndex, true, "Kicked");
            }
        }
        break;

    case COMMAND_START:
        {
            if (aMgr.getMode() == LobbyFrontendManager::Mode::HEADLESS_HOST) {
                aMgr.ccomp<MainGameplayManagerInterface>().startGame();
                aMgr._notifyAllToStart();
            } else {
                aMgr.setVisible(false);
            }
        }
        break;

    default:
        break;
    }
}

namespace {
RN_DEFINE_RPC(LobbyFrontendManager_LockInLobby, RN_ARGS(std::string&, aAuthToken)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_LOCK_IN,
                        nullptr);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface&) {
        throw RN_IllegalMessage();
    });
}

RN_DEFINE_RPC(LobbyFrontendManager_ResetLobby, RN_ARGS(std::string&, aAuthToken)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_RESET,
                        nullptr);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface&) {
        throw RN_IllegalMessage();
    });
}

RN_DEFINE_RPC(LobbyFrontendManager_MoveUp,
              RN_ARGS(std::string&, aAuthToken, hg::PZInteger, aSlotIndex)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_MOVE_UP,
                        &aSlotIndex);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface&) {
        throw RN_IllegalMessage();
    });
}

RN_DEFINE_RPC(LobbyFrontendManager_MoveDown,
              RN_ARGS(std::string&, aAuthToken, hg::PZInteger, aSlotIndex)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_MOVE_DN,
                        &aSlotIndex);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface&) {
        throw RN_IllegalMessage();
    });
}

RN_DEFINE_RPC(LobbyFrontendManager_Kick, RN_ARGS(std::string&, aAuthToken, hg::PZInteger, aSlotIndex)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_KICK,
                        &aSlotIndex);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface&) {
        throw RN_IllegalMessage();
    });
}

RN_DEFINE_RPC(LobbyFrontendManager_Start, RN_ARGS(std::string&, aAuthToken)) {
    RN_NODE_IN_HANDLER().callIfServer([&](RN_ServerInterface& aServer) {
        const spe::RPCReceiverContext rc{aServer};
        auto& authMgr = rc.gameContext.getComponent<spe::AuthorizationManagerInterface>();
        if (aAuthToken != *authMgr.getLocalAuthToken()) {
            throw RN_IllegalMessage();
        }
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_START,
                        nullptr);
    });
    RN_NODE_IN_HANDLER().callIfClient([](RN_ClientInterface& aClient) {
        const spe::RPCReceiverContext rc{aClient};
        ActivateCommand(dynamic_cast<LobbyFrontendManager&>(
                            rc.gameContext.getComponent<LobbyFrontendManagerInterface>()),
                        COMMAND_START,
                        nullptr);
    });
}
} // namespace

///////////////////////////////////////////////////////////////////////////
// MARK: IMPL                                                            //
///////////////////////////////////////////////////////////////////////////

class LobbyFrontendManager::Impl
    : hg::util::NonCopyable
    , hg::util::NonMoveable {
public:
#define CTX   _super.ctx
#define CCOMP _super.ccomp

    explicit Impl(LobbyFrontendManager& aLobbyFrontendManager)
        : _super{aLobbyFrontendManager} {}

    ~Impl() {
        if (_mode == Mode::CLIENT) {
            auto& guiContext = CCOMP<MWindow>().getGUIContext();
            if (_document) {
                guiContext.UnloadDocument(_document);
                _document = nullptr;
            }
            guiContext.RemoveDataModel("lobby-model");
        }
    }

    void setToHeadlessHostMode() {
        SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(CTX(), headless == true);
        _mode = Mode::HEADLESS_HOST;
    }

    void setToClientMode(const std::string& aName, const std::string& aUniqueId) {
        SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(CTX(), headless == false);
        _mode = Mode::CLIENT;

        {
            auto& lobbyBackendMgr = CCOMP<MLobbyBackend>();
            lobbyBackendMgr.setLocalName(aName);
            lobbyBackendMgr.setLocalUniqueId(aUniqueId);
        }

        auto& guiContext = CCOMP<MWindow>().getGUIContext();
        auto  handle     = _setUpDataBinding(guiContext);
        HG_HARD_ASSERT(handle.has_value()); // TODO
        _dataModelHandle = *handle;

        std::filesystem::path root = std::filesystem::current_path();
        for (int i = 0; i < 10; i += 1) {
            if (std::filesystem::exists(root / "Assets")) {
                break;
            }
            root = root.parent_path();
        }

        hg::rml::PreprocessRcssFile((root / "Assets/lobby.rcss.fp").string());
        _document = guiContext.LoadDocument((root / "Assets/lobby.rml").string());
        if (_document) {
            _document->Show();
            _documentVisible = true;
            HG_LOG_INFO(LOG_ID, "RMLUI Document loaded successfully.");
        } else {
            HG_LOG_ERROR(LOG_ID, "RMLUI Document could not be loaded.");
        }
    }

    Mode getMode() const {
        return _mode;
    }

    void eventBeginUpdate() {
        auto&                  lobbyBackendMgr = CCOMP<MLobbyBackend>();
        spe::LobbyBackendEvent ev;
        while (lobbyBackendMgr.pollEvent(ev)) {
            ev.strictVisit(
                [this](spe::LobbyBackendEvent::LobbyLockedIn& aEvData) {
                    HG_LOG_INFO(LOG_ID, "(event) Lobby locked in.");
                    if (_mode == Mode::CLIENT && aEvData.somethingDidChange) {
                        _document->Show();
                        _documentVisible = true;
                    }
                },
                [this](spe::LobbyBackendEvent::LobbyChanged&) {
                    HG_LOG_INFO(LOG_ID, "(event) Lobby changed.");
                    if (_mode == Mode::CLIENT) {
                        _document->Show();
                        _documentVisible = true;
                    }
                });
        }
    }

    void eventUpdate1() {
        if (_mode != Mode::CLIENT) {
            return;
        }

        auto& winMgr = CCOMP<MWindow>();
        if (winMgr.getInput().checkPressed(hg::in::PK_L, spe::WindowFrameInputView::Mode::Edge)) {
            _documentVisible = !_documentVisible;
            if (_documentVisible) {
                _document->Show();
            } else {
                _document->Hide();
            }
        }
    }

    void setVisible(bool aVisible) {
        HG_HARD_ASSERT(_mode == Mode::CLIENT);

        _documentVisible = aVisible;
        if (_documentVisible) {
            _document->Show();
        } else {
            _document->Hide();
        }
    }

    void eventDrawGUI() {
        if (_mode != Mode::CLIENT) {
            return;
        }

        const auto& lobbyBackendMgr = CCOMP<MLobbyBackend>();

        _lobbyModel.players.resize(hg::pztos(lobbyBackendMgr.getSize()));
        for (hg::PZInteger i = 0; i < lobbyBackendMgr.getSize(); i += 1) {
            const auto& lockedIn = lobbyBackendMgr.getLockedInPlayerInfo(i);
            _lobbyModel.players[hg::pztos(i)].lockedIn =
                PlayerInfoModel{(!lockedIn.name.empty()) ? lockedIn.name : "<empty>",
                                lockedIn.uniqueId,
                                lockedIn.ipAddress};

            if (lobbyBackendMgr.areChangesPending(i)) {
                _lobbyModel.players[hg::pztos(i)].showPending = true;
                const auto& pending                           = lobbyBackendMgr.getPendingPlayerInfo(i);
                _lobbyModel.players[hg::pztos(i)].pending =
                    PlayerInfoModel{(!pending.name.empty()) ? pending.name : "<empty>",
                                    pending.uniqueId,
                                    pending.ipAddress};
            } else {
                _lobbyModel.players[hg::pztos(i)].showPending = false;
            }
        }

        _lobbyModel.localName = lobbyBackendMgr.getLocalName();
        _lobbyModel.isAuthorized =
            CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken().has_value();

        _dataModelHandle.DirtyAllVariables();
    }

    void notifyAllToStart() {
        const auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken();
        if (authToken.has_value()) {
            Compose_LobbyFrontendManager_Start(CCOMP<MNetworking>().getNode(),
                                               RN_COMPOSE_FOR_ALL,
                                               *authToken);
        }
    }

private:
    LobbyFrontendManager& _super;

    Mode _mode = Mode::UNINITIALIZED;

    Rml::ElementDocument* _document = nullptr;
    LobbyModel            _lobbyModel;
    Rml::DataModelHandle  _dataModelHandle;

    bool _documentVisible = false;

    void _onLockInClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_LockInLobby(CCOMP<MNetworking>().getNode(),
                                                     RN_COMPOSE_FOR_ALL,
                                                     *authToken);
        }
    }

    void _onResetClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_ResetLobby(CCOMP<MNetworking>().getNode(),
                                                    RN_COMPOSE_FOR_ALL,
                                                    *authToken);
        }
    }

    void _onMoveUpClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_MoveUp(CCOMP<MNetworking>().getNode(),
                                                RN_COMPOSE_FOR_ALL,
                                                *authToken,
                                                aArguments.at(0).Get<int>(-1));
        }
    }

    void _onMoveDownClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_MoveDown(CCOMP<MNetworking>().getNode(),
                                                  RN_COMPOSE_FOR_ALL,
                                                  *authToken,
                                                  aArguments.at(0).Get<int>(-1));
        }
    }

    void _onKickClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_Kick(CCOMP<MNetworking>().getNode(),
                                              RN_COMPOSE_FOR_ALL,
                                              *authToken,
                                              aArguments.at(0).Get<int>(-1));
        }
    }

    void _onStartClicked(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        if (auto authToken = CCOMP<spe::AuthorizationManagerInterface>().getLocalAuthToken()) {
            Compose_LobbyFrontendManager_Start(CCOMP<MNetworking>().getNode(),
                                               RN_COMPOSE_FOR_ALL,
                                               *authToken);
        }
    }

    std::optional<Rml::DataModelHandle> _setUpDataBinding(Rml::Context& aContext) {
        Rml::DataModelConstructor constructor = aContext.CreateDataModel("lobby-model");
        if (!constructor) {
            return {};
        }

        // clang-format off
        try {
            #define THROW_IF_FALSE(_val_)                                                              \
                do {                                                                                   \
                    if (!(_val_)) {                                                                    \
                        HG_THROW_TRACED(hg::TracedRuntimeError, 0, "Expression failed: '{}'", #_val_); \
                    }                                                                                  \
                } while (0)

            THROW_IF_FALSE(RegisterModel<PlayerInfoModel>(constructor));
            THROW_IF_FALSE(RegisterModel<DualPlayerInfoModel>(constructor));
            THROW_IF_FALSE(constructor.RegisterArray<std::vector<DualPlayerInfoModel>>());
            THROW_IF_FALSE(RegisterModel<LobbyModel>(constructor));
        
            THROW_IF_FALSE(constructor.Bind("players",      &_lobbyModel.players));
            THROW_IF_FALSE(constructor.Bind("localName",    &_lobbyModel.localName));
            THROW_IF_FALSE(constructor.Bind("isAuthorized", &_lobbyModel.isAuthorized));

            THROW_IF_FALSE(constructor.BindEventCallback("LockIn",   &Impl::_onLockInClicked,   this));
            THROW_IF_FALSE(constructor.BindEventCallback("Reset",    &Impl::_onResetClicked,    this));
            THROW_IF_FALSE(constructor.BindEventCallback("MoveUp",   &Impl::_onMoveUpClicked,   this));
            THROW_IF_FALSE(constructor.BindEventCallback("MoveDown", &Impl::_onMoveDownClicked, this));
            THROW_IF_FALSE(constructor.BindEventCallback("Kick",     &Impl::_onKickClicked,     this));
            THROW_IF_FALSE(constructor.BindEventCallback("Start",    &Impl::_onStartClicked,    this));     
        } catch (const hg::TracedRuntimeError& ex) {
            HG_LOG_ERROR(LOG_ID, "Could not bind data model: {}", ex.getErrorMessage());
            return {};
        }
        // clang-format on

        return constructor.GetModelHandle();
    }

#undef CCOMP
#undef CTX
};

///////////////////////////////////////////////////////////////////////////
// MARK: LOBBY FRONTEND MANAGER                                          //
///////////////////////////////////////////////////////////////////////////

LobbyFrontendManager::LobbyFrontendManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : NonstateObject(aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "LobbyFrontendManager")
    , _impl{std::make_unique<Impl>(*this)} {}

LobbyFrontendManager::~LobbyFrontendManager() = default;

void LobbyFrontendManager::setToHeadlessHostMode() {
    _impl->setToHeadlessHostMode();
}

void LobbyFrontendManager::setToClientMode(const std::string& aName, const std::string& aUniqueId) {
    _impl->setToClientMode(aName, aUniqueId);
}

LobbyFrontendManager::Mode LobbyFrontendManager::getMode() const {
    return _impl->getMode();
}

void LobbyFrontendManager::setVisible(bool aVisible) {
    _impl->setVisible(aVisible);
}

void LobbyFrontendManager::_eventBeginUpdate() {
    _impl->eventBeginUpdate();
}

void LobbyFrontendManager::_eventUpdate1() {
    _impl->eventUpdate1();
}

void LobbyFrontendManager::_eventDrawGUI() {
    _impl->eventDrawGUI();
}

void LobbyFrontendManager::_notifyAllToStart() {
    _impl->notifyAllToStart();
}
