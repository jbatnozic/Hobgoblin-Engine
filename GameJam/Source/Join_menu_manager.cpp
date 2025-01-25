#include "Join_menu_manager.hpp"

#include "Context_factory.hpp"
#include "Host_menu_manager_interface.hpp"
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
    HG_UNREACHABLE();
    return false;
}

struct JoinMenuModel {
    bool testMode = false;

    /**
     * 0 = disabled
     * 1 = initializing
     * 2 = failure
     * 3 = active
     */
    int zeroTierStatus = 0;
};

template <>
bool RegisterModel<JoinMenuModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<JoinMenuModel>();
    if (handle) {
        handle.RegisterMember<bool>("testMode", &JoinMenuModel::testMode);
        handle.RegisterMember<int>("zeroTierStatus", &JoinMenuModel::zeroTierStatus);
    }
    return static_cast<bool>(handle);
}
} // namespace

class JoinMenuManager::Impl
    : hg::util::NonCopyable
    , hg::util::NonMoveable
    , Rml::EventListener {
public:
#define CTX   _super.ctx
#define CCOMP _super.ccomp

    Impl(JoinMenuManager& aSuper)
        : _super{aSuper} {
        _init();
    }

    ~Impl() {
        auto& guiContext = CCOMP<MWindow>().getGUIContext();
        if (_document) {
            auto* form = _document->GetElementById("join-form");
            if (form) {
                form->RemoveEventListener("submit", this);
            }

            guiContext.UnloadDocument(_document);
            _document = nullptr;
        }
        guiContext.RemoveDataModel("joinmenu-model");
    }

    void ProcessEvent(Rml::Event& aEvent) override {
        const auto playerName = aEvent.GetParameter<Rml::String>("playerName", "");
        const auto serverIp   = aEvent.GetParameter<Rml::String>("serverIp", "");
        const auto serverPort = aEvent.GetParameter<Rml::String>("serverPort", "");
        const auto localPort  = aEvent.GetParameter<Rml::String>("localPort", "");
        this->onFormSubmitted(playerName, serverIp, serverPort, localPort);
    }

    void onFormSubmitted(const Rml::String& aPlayerName,
                         const Rml::String& aServerIp,
                         const Rml::String& aServerPort,
                         const Rml::String& aLocalPort) {
        HG_LOG_INFO(
            LOG_ID,
            "Connect form submitted: playerName: {}, serverIp: {}, serverPort: {}, localPort: {}",
            aPlayerName,
            aServerIp,
            aServerPort,
            aLocalPort);

        ClientGameParams params{.playerName      = aPlayerName,
                                .hostIpAddress   = aServerIp,
                                .hostPortNumber  = static_cast<std::uint16_t>(std::stoi(aServerPort)),
                                .localPortNumber = static_cast<std::uint16_t>(
                                    (aLocalPort == "auto") ? 0 : std::stoi(aLocalPort)),
                                .zeroTierEnabled = _super._zeroTierEnabled,
                                .skipConnect     = false};

        _super._clientGameParams = std::make_unique<ClientGameParams>(std::move(params));
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
                if (_joinMenuModel.zeroTierStatus != 2) {
                    _joinMenuModel.zeroTierStatus = 2;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::INITIALIZING:
                if (_joinMenuModel.zeroTierStatus != 1) {
                    _joinMenuModel.zeroTierStatus = 1;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::FAILURE:
                if (_joinMenuModel.zeroTierStatus != 2) {
                    _joinMenuModel.zeroTierStatus = 2;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            case SimpleZeroTier_Status::ACTIVE:
                if (_joinMenuModel.zeroTierStatus != 3) {
                    _joinMenuModel.zeroTierStatus = 3;
                    _dataModelHandle.DirtyAllVariables();
                }
                break;

            default:
                HG_UNREACHABLE();
            }
        } else {
            if (_joinMenuModel.zeroTierStatus != 0) {
                _joinMenuModel.zeroTierStatus = 0;
                _dataModelHandle.DirtyAllVariables();
            }
        }
    }

private:
    JoinMenuManager& _super;

    Rml::ElementDocument* _document = nullptr;
    JoinMenuModel         _joinMenuModel;
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

        hg::rml::PreprocessRcssFile((root / "Assets/join_menu.rcss.fp").string());
        _document = guiContext.LoadDocument((root / "Assets/join_menu.rml").string());

        auto* form = _document->GetElementById("join-form");
        if (form) {
            HG_LOG_INFO(LOG_ID, "Found join-form");
            form->AddEventListener("submit", this);
        }

        if (_document) {
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
        Rml::DataModelConstructor constructor = aContext.CreateDataModel("joinmenu-model");
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

            THROW_IF_FALSE(RegisterModel<JoinMenuModel>(constructor)); 
            THROW_IF_FALSE(constructor.Bind("testMode", &_joinMenuModel.testMode));
            THROW_IF_FALSE(constructor.Bind("zeroTierStatus", &_joinMenuModel.zeroTierStatus));
            THROW_IF_FALSE(constructor.BindEventCallback("Back",   &Impl::_onBack,   this));   
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

JoinMenuManager::JoinMenuManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : spe::NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "JoinMenuManager"}
    , _impl{std::make_unique<Impl>(*this)} {}

JoinMenuManager::~JoinMenuManager() = default;

void JoinMenuManager::setVisible(bool aVisible) {
    _impl->setVisible(aVisible);
}

void JoinMenuManager::_eventPreUpdate() {
    if (_timeToDie && _clientGameParams) {
        _impl.reset();

        // Start game
        AttachGameplayManagers(ctx(), *_clientGameParams);

        spe::DetachStatus detachStatus;

        // Detach & destroy main menu manager
        {
            auto mainMenuMgr = ctx().detachComponent<MainMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && mainMenuMgr != nullptr);
            mainMenuMgr.reset();
        }
        // Detach & destroy host menu manager
        {
            auto hostMenuMgr = ctx().detachComponent<HostMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && hostMenuMgr != nullptr);
            hostMenuMgr.reset();
        }
        // Detach & destroy join menu manager (self)
        {
            auto self = ctx().detachComponent<JoinMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && self != nullptr);
            self.reset(); // WARNING: `this` will be deleted!
        }
    }
}

void JoinMenuManager::_eventDrawGUI() {
    _impl->eventDrawGUI();
}
