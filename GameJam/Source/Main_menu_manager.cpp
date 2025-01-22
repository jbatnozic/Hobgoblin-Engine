#include "Main_menu_manager.hpp"

#include "Context_factory.hpp"
#include "Host_menu_manager_interface.hpp"
#include "Join_menu_manager_interface.hpp"
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

struct MainMenuModel {
    bool testMode = false;
};

template <>
bool RegisterModel<MainMenuModel>(Rml::DataModelConstructor& aDataModelCtor) {
    auto handle = aDataModelCtor.RegisterStruct<MainMenuModel>();
    if (handle) {
        handle.RegisterMember<bool>("testMode", &MainMenuModel::testMode);
    }
    return static_cast<bool>(handle);
}
} // namespace

class MainMenuManager::Impl
    : hg::util::NonCopyable
    , hg::util::NonMoveable {
public:
#define CTX   _super.ctx
#define CCOMP _super.ccomp

    Impl(MainMenuManager& aSuper)
        : _super{aSuper} {
        _init();
    }

    ~Impl() {
        auto& guiContext = CCOMP<MWindow>().getGUIContext();
        if (_document) {
            guiContext.UnloadDocument(_document);
            _document = nullptr;
        }
        guiContext.RemoveDataModel("mainmenu-model");
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
        // if (_mainMenuModel.testMode != testMode) {
        //     _mainMenuModel.testMode = testMode;
        //     _dataModelHandle.DirtyAllVariables();
        // }
    }

private:
    MainMenuManager& _super;

    Rml::ElementDocument* _document = nullptr;
    MainMenuModel         _mainMenuModel;
    Rml::DataModelHandle  _dataModelHandle;

    bool _documentVisible = false;

    void _init() {
        SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(CTX(), headless == false);

        auto& guiContext = CCOMP<MWindow>().getGUIContext();
        auto  handle     = _setUpDataBinding(guiContext);
        HG_HARD_ASSERT(handle.has_value()); // TODO
        _dataModelHandle = *handle;

        hg::rml::PreprocessRcssFile("Assets/main_menu.rcss.fp");
        _document = guiContext.LoadDocument("Assets/main_menu.rml");

        if (_document) {
            _document->Show();
            _documentVisible = true;
            HG_LOG_INFO(LOG_ID, "RMLUI Document loaded successfully.");
        } else {
            HG_LOG_ERROR(LOG_ID, "RMLUI Document could not be loaded.");
        }
    }

    std::optional<Rml::DataModelHandle> _setUpDataBinding(Rml::Context& aContext) {
        Rml::DataModelConstructor constructor = aContext.CreateDataModel("mainmenu-model");
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

            THROW_IF_FALSE(RegisterModel<MainMenuModel>(constructor));
            THROW_IF_FALSE(constructor.Bind("testMode", &_mainMenuModel.testMode));
            THROW_IF_FALSE(constructor.BindEventCallback("Solo",             &Impl::_onSolo,             this));
            THROW_IF_FALSE(constructor.BindEventCallback("Host",             &Impl::_onHost,             this));
            THROW_IF_FALSE(constructor.BindEventCallback("HostWithZeroTier", &Impl::_onHostWithZeroTier, this));
            THROW_IF_FALSE(constructor.BindEventCallback("Join",             &Impl::_onJoin,             this));
            THROW_IF_FALSE(constructor.BindEventCallback("JoinWithZeroTier", &Impl::_onJoinWithZeroTier, this));
            THROW_IF_FALSE(constructor.BindEventCallback("Exit",             &Impl::_onExit,             this));
        } catch (const hg::TracedRuntimeError& ex) {
            HG_LOG_ERROR(LOG_ID, "Could not bind data model: {}", ex.getErrorMessage());
            return {};
        }
        // clang-format on

        return constructor.GetModelHandle();
    }

    void _onSolo(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        // clang-format off
        ServerGameParams serverParams{
            .playerCount     = 2,
            .portNumber      = 8889,
            .zeroTierEnabled = false
        };

        ClientGameParams clientParams{
            .playerName      = GetRandomGymName(),
            .hostIpAddress   = "127.0.0.1",
            .hostPortNumber  = 8889,
            .localPortNumber = 0,
            .zeroTierEnabled = false,
            .skipConnect     = true
        };
        // clang-format on

        _super._serverGameParams = std::make_unique<ServerGameParams>(std::move(serverParams));
        _super._clientGameParams = std::make_unique<ClientGameParams>(std::move(clientParams));
        _super._timeToDie        = true;
    }

    void _onHost(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        CCOMP<HostMenuManagerInterface>().setVisible(true);
        CCOMP<HostMenuManagerInterface>().setZeroTierEnabled(false);
        this->setVisible(false);
    }

    void _onHostWithZeroTier(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        _initZeroTier();
        CCOMP<HostMenuManagerInterface>().setVisible(true);
        CCOMP<HostMenuManagerInterface>().setZeroTierEnabled(true);
        this->setVisible(false);
    }

    void _onJoin(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        CCOMP<JoinMenuManagerInterface>().setVisible(true);
        CCOMP<JoinMenuManagerInterface>().setZeroTierEnabled(false);
        this->setVisible(false);
    }

    void _onJoinWithZeroTier(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        _initZeroTier();
        CCOMP<JoinMenuManagerInterface>().setVisible(true);
        CCOMP<JoinMenuManagerInterface>().setZeroTierEnabled(true);
        this->setVisible(false);
    }

    void _onExit(Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& aArguments) {
        CTX().stop();
    }

    void _initZeroTier() {
        if (SimpleZeroTier_Status() == SimpleZeroTier_Status::STOPPED) {
            SimpleZeroTier_Init("ztNodeIdentity", 8989, 0xd3ecf5726d81ccb3, std::chrono::seconds{20});
        }
    }

#undef CCOMP
#undef CTX
};

MainMenuManager::MainMenuManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : spe::NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "JoinMenuManager"}
    , _impl{std::make_unique<Impl>(*this)} {}

MainMenuManager::~MainMenuManager() = default;

void MainMenuManager::setVisible(bool aVisible) {
    _impl->setVisible(aVisible);
}

void MainMenuManager::_eventPreUpdate() {
    if (_timeToDie && _serverGameParams && _clientGameParams) {
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

        // Detach & destroy host menu manager
        {
            auto hostMenuMgr = ctx().detachComponent<HostMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && hostMenuMgr != nullptr);
            hostMenuMgr.reset();
        }
        // Detach & destroy join menu manager
        {
            auto joinMenuMgr = ctx().detachComponent<JoinMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && joinMenuMgr != nullptr);
            joinMenuMgr.reset();
        }
        // Detach & destroy main menu manager (self)
        {
            auto self = ctx().detachComponent<MainMenuManagerInterface>(&detachStatus);
            HG_HARD_ASSERT(detachStatus == spe::DetachStatus::OK && self != nullptr);
            self.reset(); // WARNING: `this` will be deleted!
        }
    }
}

void MainMenuManager::_eventDrawGUI() {
    _impl->eventDrawGUI();
}
