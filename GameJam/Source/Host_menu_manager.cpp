#include "Host_menu_manager.hpp"

#include "Context_factory.hpp"
#include "Join_menu_manager_interface.hpp"
#include "Main_menu_manager_interface.hpp"
#include "Names.hpp"
#include "Simple_zerotier.hpp"

#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/Utility/No_copy_no_move.hpp>
#include <Hobgoblin/Utility/Randomization.hpp>
#include <RmlUi/Core.h>

#include <array>
#include <string>

namespace {
template <class T>
bool RegisterModel(Rml::DataModelConstructor& aDataModelCtor) {
    return false;
}

struct HostMenuModel {
    bool testMode = false;

    /**
     * 0 = disabled
     * 1 = initializing
     * 2 = failure
     * 3 = active
     */
    int         zeroTierStatus = 0;
    Rml::String localIpAddress = "n/a";
};

template <>
bool RegisterModel<HostMenuModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<HostMenuModel>();
    if (handle) {
        handle.RegisterMember<bool>("testMode", &HostMenuModel::testMode);
        handle.RegisterMember<int>("zeroTierStatus", &HostMenuModel::zeroTierStatus);
        handle.RegisterMember<Rml::String>("localIpAddress", &HostMenuModel::localIpAddress);
    }
    return static_cast<bool>(handle);
}
} // namespace

class HostMenuManager::Impl
    : hg::util::NonCopyable
    , hg::util::NonMoveable
    , Rml::EventListener {
public:
#define CTX   _super.ctx
#define CCOMP _super.ccomp

    Impl(HostMenuManager& aSuper)
        : _super{aSuper} {
        _init();
    }

    ~Impl() {
        auto& guiContext = CCOMP<MWindow>().getGUIContext();
        if (_document) {
            auto* form = _document->GetElementById("host-form");
            if (form) {
                form->RemoveEventListener("submit", this);
            }

            guiContext.UnloadDocument(_document);
            _document = nullptr;
        }
        guiContext.RemoveDataModel("hostmenu-model");
    }

    void ProcessEvent(Rml::Event& aEvent) override {
        const auto playerName  = aEvent.GetParameter<Rml::String>("playerName", "");
        const auto playerCount = aEvent.GetParameter<Rml::String>("playerCount", "");
        const auto serverPort  = aEvent.GetParameter<Rml::String>("serverPort", "");
        this->onFormSubmitted(playerName, playerCount, serverPort);
    }

    void onFormSubmitted(const Rml::String& aPlayerName,
                         const Rml::String& aPlayerCount,
                         const Rml::String& aServerPort) {
        HG_LOG_INFO(LOG_ID,
                    "Host form submitted: playerName: {}, playerCount: {}, serverPort: {}",
                    aPlayerName,
                    aPlayerCount,
                    aServerPort);

        // clang-format off
        ServerGameParams serverParams{
            .playerCount     = std::stoi(aPlayerCount) + 1,
            .portNumber      = static_cast<std::uint16_t>(std::stoi(aServerPort)),
            .zeroTierEnabled = _super._zeroTierEnabled
        };

        ClientGameParams clientParams{
            .playerName      = aPlayerName,
            .hostIpAddress   = "127.0.0.1",
            .hostPortNumber  = static_cast<std::uint16_t>(std::stoi(aServerPort)),
            .localPortNumber = 0,
            .zeroTierEnabled = _super._zeroTierEnabled,
            .skipConnect     = true
        };
        // clang-format on

        _super._serverGameParams = std::make_unique<ServerGameParams>(std::move(serverParams));
        _super._clientGameParams = std::make_unique<ClientGameParams>(std::move(clientParams));
        _super._timeToDie        = true;
    }

    void setVisible(bool aVisible) {
        _documentVisible = aVisible;
        if (_documentVisible) {
            _document->Show();
        } else {
            _document->Hide();
        }
    }

    void eventDrawGUI() {
        // const bool testMode = CTX().hasChildContext();
        // if (_hostMenuModel.testMode != testMode) {
        //     _hostMenuModel.testMode = testMode;
        //     _dataModelHandle.DirtyAllVariables();
        // }

        if (_super._zeroTierEnabled) {
            switch (SimpleZeroTier_GetStatus()) {
            case SimpleZeroTier_Status::STOPPED:
                if (_hostMenuModel.zeroTierStatus != 2) {
                    _hostMenuModel.zeroTierStatus = 2;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::INITIALIZING:
                if (_hostMenuModel.zeroTierStatus != 1) {
                    _hostMenuModel.zeroTierStatus = 1;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::FAILURE:
                if (_hostMenuModel.zeroTierStatus != 2) {
                    _hostMenuModel.zeroTierStatus = 2;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::ACTIVE:
                if (_hostMenuModel.zeroTierStatus != 3) {
                    _hostMenuModel.zeroTierStatus = 3;
                    _hostMenuModel.localIpAddress = SimpleZeroTier_GetLocalIpAddress().toString();
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            default:
                HG_UNREACHABLE();
            }
        } else {
            if (_hostMenuModel.zeroTierStatus != 0) {
                _hostMenuModel.zeroTierStatus = 0;
                _dataModelHandle.DirtyAllVariables();
            }
        }
    }

private:
    HostMenuManager& _super;

    Rml::ElementDocument* _document = nullptr;
    HostMenuModel         _hostMenuModel;
    Rml::DataModelHandle  _dataModelHandle;

    bool _documentVisible = false;

    void _init() {
        SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(CTX(), headless == false);

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

        hg::rml::PreprocessRcssFile((root / "Assets/host_menu.rcss.fp").string());
        _document = guiContext.LoadDocument((root / "Assets/host_menu.rml").string());

        if (_document) {
            auto* form = _document->GetElementById("host-form");
            if (form) {
                HG_LOG_INFO(LOG_ID, "Found host-form");
                form->AddEventListener("submit", this);
            }

            auto* nameElement = _document->GetElementById("playerName");
            if (nameElement) {
                nameElement->SetAttribute("value", GetRandomGymName());
            }

            _document->Show();

            _documentVisible = true;
            HG_LOG_INFO(LOG_ID, "RMLUI Document loaded successfully.");
        } else {
            HG_LOG_ERROR(LOG_ID, "RMLUI Document could not be loaded.");
        }
    }

    std::optional<Rml::DataModelHandle> _setUpDataBinding(Rml::Context& aContext) {
        Rml::DataModelConstructor constructor = aContext.CreateDataModel("hostmenu-model");
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

            THROW_IF_FALSE(RegisterModel<HostMenuModel>(constructor));
            THROW_IF_FALSE(constructor.Bind("testMode", &_hostMenuModel.testMode));
            THROW_IF_FALSE(constructor.Bind("zeroTierStatus", &_hostMenuModel.zeroTierStatus));
            THROW_IF_FALSE(constructor.Bind("localIpAddress", &_hostMenuModel.localIpAddress));
            THROW_IF_FALSE(constructor.BindEventCallback("Back", &Impl::_onBack,   this));         
        } catch (const hg::TracedRuntimeError& ex) {
            HG_LOG_ERROR(LOG_ID, "Could not bind data model: {}", ex.getErrorMessage());
            return {};
        }
        // clang-format on

        return constructor.GetModelHandle();
    }

    void _onBack(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        CCOMP<MainMenuManagerInterface>().setVisible(true);
        this->setVisible(false);
    }

#undef CCOMP
#undef CTX
};

HostMenuManager::HostMenuManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : spe::NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "HostMenuManager"}
    , _impl{std::make_unique<Impl>(*this)} {}

HostMenuManager::~HostMenuManager() = default;

void HostMenuManager::setVisible(bool aVisible) {
    _impl->setVisible(aVisible);
}

void HostMenuManager::_eventPreUpdate() {
    if (_timeToDie && _clientGameParams && _serverGameParams) {
        _impl.reset();

        // Start game
        ctx().attachChildContext(CreateServerContext(*_serverGameParams));
        AttachGameplayManagers(ctx(), *_clientGameParams);
        if (_clientGameParams->skipConnect) {
            auto& server = ctx().getChildContext()->getComponent<MNetworking>().getServer();
            auto& client = ccomp<MNetworking>().getClient();
            client.connectLocal(server);
        }
        ctx().startChildContext(-1);

        spe::DetachStatus detachStatus;

        // Detach & destroy main menu manager
        {
            auto mainMenuMgr = ctx().detachComponent<MainMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && mainMenuMgr != nullptr);
            mainMenuMgr.reset();
        }
        // Detach & destroy join menu manager
        {
            auto joinMenuMgr = ctx().detachComponent<JoinMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && joinMenuMgr != nullptr);
            joinMenuMgr.reset();
        }
        // Detach & destroy host menu manager (self)
        {
            auto self = ctx().detachComponent<HostMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && self != nullptr);
            self.reset(); // WARNING: `this` will be deleted!
        }
    }
}

void HostMenuManager::_eventDrawGUI() {
    _impl->eventDrawGUI();
}
