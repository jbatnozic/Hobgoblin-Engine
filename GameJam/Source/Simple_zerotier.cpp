#include "Simple_zerotier.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>

#include <ZTCpp.hpp>

#include <chrono>
#include <mutex>
#include <thread>

namespace {
constexpr auto LOG_ID = "SPeMPE.ManualTest";

namespace zt = jbatnozic::ztcpp;

class NodeInfo {
public:
    NodeInfo() = default;

    bool          online              = false;
    int           networksJoinedCount = 0;
    uint64_t      id                  = 0;
    zt::IpAddress ip4                 = zt::IpAddress::ipv4Unspecified();
};

class ZeroTierEventHandler : public zt::EventHandlerInterface {
public:
    ZeroTierEventHandler(NodeInfo& aNodeInfo)
        : _nodeInfo{aNodeInfo} {}

    void onAddressEvent(zt::EventCode::Address    aEventCode,
                        const zt::AddressDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));

        if (aDetails && aEventCode == zt::EventCode::Address::AddedIPv4) {
            _nodeInfo.ip4 = aDetails->getIpAddress();
        }
    }

    void onNetworkEvent(zt::EventCode::Network    aEventCode,
                        const zt::NetworkDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));

        if (aEventCode == zt::EventCode::Network::ReadyIPv4 ||
            aEventCode == zt::EventCode::Network::ReadyIPv6 ||
            aEventCode == zt::EventCode::Network::ReadyIPv4_IPv6) {
            _nodeInfo.networksJoinedCount += 1;
        } else if (aEventCode == zt::EventCode::Network::Down) {
            _nodeInfo.networksJoinedCount -= 1;
        }
    }

    void onNetworkInterfaceEvent(zt::EventCode::NetworkInterface    aEventCode,
                                 const zt::NetworkInterfaceDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));
    }

    void onNetworkStackEvent(zt::EventCode::NetworkStack    aEventCode,
                             const zt::NetworkStackDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));
    }

    void onNodeEvent(zt::EventCode::Node aEventCode, const zt::NodeDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));

        if (aEventCode == zt::EventCode::Node::Online) {
            _nodeInfo.online = true;
        } else if (aEventCode == zt::EventCode::Node::Online) {
            _nodeInfo.online = false;
        }
    }

    void onPeerEvent(zt::EventCode::Peer aEventCode, const zt::PeerDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));
    }

    void onRouteEvent(zt::EventCode::Route    aEventCode,
                      const zt::RouteDetails* aDetails) noexcept override {
        HG_LOG_INFO(LOG_ID, "{}", zt::EventDescription(aEventCode, aDetails));
    }

    void onUnknownEvent(int16_t aRawZeroTierEventCode) noexcept override {
        HG_LOG_WARN(LOG_ID, "Unknown event dispatched ({}).", aRawZeroTierEventCode);
    }

private:
    NodeInfo& _nodeInfo;
};

#define DEFER(...) __VA_ARGS__

#define THROW_TRACED_ON_ZTCPP_ERROR(_result_, _exception_type_)    \
    do {                                                           \
        if ((_result_).hasError()) {                               \
            DEFER(HG_THROW_TRACED(_exception_type_,                \
                                  (_result_).getError().errorCode, \
                                  (_result_).getError().message)); \
        }                                                          \
    } while (false)

class SimpleZeroTierInitializer {
public:
    ~SimpleZeroTierInitializer() {
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _stopASAP = true;
        }
        if (_worker.joinable()) {
            _worker.join();
        }
        stop();
    }

    static SimpleZeroTierInitializer& getInstance() {
        static SimpleZeroTierInitializer singleton;
        return singleton;
    }

    void initialize(const std::string&        aNodeIdentityPath,
                    std::uint16_t             aServicePort,
                    std::uint64_t             aNetworkId,
                    std::chrono::milliseconds aMaxTimeToWait) {
        std::unique_lock<std::mutex> lock{_mutex};
        if (_status == SimpleZeroTier_Status::INITIALIZING) {
            return;
        }
        
        HG_VALIDATE_PRECONDITION(_status == SimpleZeroTier_Status::STOPPED);

        _worker = std::thread{[=, this]() {
            _initializeImpl(aNodeIdentityPath, aServicePort, aNetworkId, aMaxTimeToWait);
        }};
    }

    SimpleZeroTier_Status getStatus() const {
        std::unique_lock<std::mutex> lock{_mutex};
        return _status;
    }

    zt::IpAddress getIpAddress() const {
        std::unique_lock<std::mutex> lock{_mutex};
        HG_VALIDATE_PRECONDITION(_status == SimpleZeroTier_Status::ACTIVE);
        return *_ipAddress;
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _status = SimpleZeroTier_Status::STOPPED;
        }
        zt::LocalNode::stop();
        zt::LocalNode::setEventHandler(nullptr);
    }

private:
    mutable std::mutex _mutex;

    SimpleZeroTier_Status        _status = SimpleZeroTier_Status::STOPPED;
    NodeInfo                     _ztLocalNodeInfo;
    ZeroTierEventHandler         _ztEventHandler{_ztLocalNodeInfo};
    std::optional<zt::IpAddress> _ipAddress;

    bool        _stopASAP = false;
    std::thread _worker;

    void _initializeImpl(const std::string         aNodeIdentityPath,
                         std::uint16_t             aServicePort,
                         std::uint64_t             aNetworkId,
                         std::chrono::milliseconds aMaxTimeToWait) {
        {
            std::unique_lock<std::mutex> lock{_mutex};
            HG_VALIDATE_PRECONDITION(_status == SimpleZeroTier_Status::STOPPED);
            _status = SimpleZeroTier_Status::INITIALIZING;
        }

        const auto startTime = std::chrono::steady_clock::now();

        HG_LOG_INFO(LOG_ID, "SimpleZeroTier -- STARTING CONFIGURATION");

        HG_LOG_INFO(LOG_ID, "Configuring service...");
        {
            const auto res = zt::Config::setIdentityFromStorage(aNodeIdentityPath);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }
        {
            const auto res = zt::Config::allowNetworkCaching(true);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }
        {
            const auto res = zt::Config::allowPeerCaching(true);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }
        {
            const auto res = zt::Config::allowIdentityCaching(true);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }
        {
            const auto res = zt::Config::setPort(aServicePort);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }

        HG_LOG_INFO(LOG_ID, "Starting service...");
        {
            zt::LocalNode::setEventHandler(&_ztEventHandler);
            const auto res = zt::LocalNode::start();
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }

        while (!_ztLocalNodeInfo.online) {
            HG_LOG_INFO(LOG_ID, "Waiting for local node to come online...");
            std::this_thread::sleep_for(std::chrono::milliseconds{500});

            if (_stopASAP) {
                return;
            }

            if (_ztLocalNodeInfo.online) {
                break;
            }

            if (std::chrono::steady_clock::now() - startTime > aMaxTimeToWait) {
                HG_LOG_INFO(LOG_ID,
                            "Timed out while waiting for local node to come online; quitting...");
                {
                    std::unique_lock<std::mutex> lock{_mutex};
                    HG_VALIDATE_PRECONDITION(_status == SimpleZeroTier_Status::INITIALIZING);
                    _status = SimpleZeroTier_Status::FAILURE;
                }
                return;
            }
        }

        HG_LOG_INFO(LOG_ID, "Joining network {:#x}", aNetworkId);
        HG_LOG_INFO(LOG_ID, "Don't forget to authorize this device in my.zerotier.com or the web API!");
        {
            const auto res = zt::Network::join(aNetworkId);
            THROW_TRACED_ON_ZTCPP_ERROR(res, SimpleZeroTierError);
        }

        while (_ztLocalNodeInfo.networksJoinedCount <= 0) {
            HG_LOG_INFO(LOG_ID, "Waiting to join network...");
            std::this_thread::sleep_for(std::chrono::milliseconds{500});

            if (_stopASAP) {
                return;
            }

            if (_ztLocalNodeInfo.networksJoinedCount > 0) {
                break;
            }

            if (std::chrono::steady_clock::now() - startTime > aMaxTimeToWait) {
                HG_LOG_INFO(LOG_ID, "Timed out while waiting to join network; quitting...");
                {
                    std::unique_lock<std::mutex> lock{_mutex};
                    HG_VALIDATE_PRECONDITION(_status == SimpleZeroTier_Status::INITIALIZING);
                    _status = SimpleZeroTier_Status::FAILURE;
                }
                return;
            }
        }

        static constexpr auto BANNER =
            "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

        HG_LOG_INFO(LOG_ID,
                    "\n{}\n" // banner
                    "* The app has been cussessfully configured to communicate via ZeroTier!\n"
                    "* IP address of the local node on the ZT network is displayed below:\n"
                    "* {}\n"
                    "{}\n", // banner
                    BANNER,
                    _ztLocalNodeInfo.ip4.toString(),
                    BANNER);

        HG_LOG_INFO(LOG_ID, "SimpleZeroTier -- CONFIGURATION FINISHED");

        {
            std::unique_lock<std::mutex> lock{_mutex};
            _ipAddress = _ztLocalNodeInfo.ip4;
            _status    = SimpleZeroTier_Status::ACTIVE;
        }
    }
};

} // namespace

void SimpleZeroTier_Init(const std::string&        aNodeIdentityPath,
                         std::uint16_t             aServicePort,
                         std::uint64_t             aNetworkId,
                         std::chrono::milliseconds aMaxTimeToWait) {
    if (SimpleZeroTier_GetStatus() == SimpleZeroTier_Status::ACTIVE) {
        return;
    }

    SimpleZeroTierInitializer::getInstance().initialize(aNodeIdentityPath,
                                                        aServicePort,
                                                        aNetworkId,
                                                        aMaxTimeToWait);
}

SimpleZeroTier_Status SimpleZeroTier_GetStatus() {
    return SimpleZeroTierInitializer::getInstance().getStatus();
}

jbatnozic::ztcpp::IpAddress SimpleZeroTier_GetLocalIpAddress() {
    return SimpleZeroTierInitializer::getInstance().getIpAddress();
}

void SimpleZeroTier_Stop() {
    if (SimpleZeroTier_GetStatus() == SimpleZeroTier_Status::STOPPED) {
        return;
    }

    SimpleZeroTierInitializer::getInstance().stop();
}
