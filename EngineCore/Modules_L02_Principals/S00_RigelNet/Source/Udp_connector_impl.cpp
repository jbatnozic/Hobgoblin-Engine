// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include "Udp_connector_impl.hpp"
#include "Invalid_data_error.hpp"
#include "Udp_connector_packet_kinds.hpp"
#include "Udp_server_impl.hpp"

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Format.hpp>
#include <Hobgoblin/HGExcept.hpp>

#include <cassert>
#include <chrono>
#include <cstring>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace rn {

namespace {
constexpr auto LOG_ID                = "Hobgoblin.RigelNet";
constexpr auto UDP_HEADER_BYTE_COUNT = 8u;
} // namespace

//! Class used when two Connectors are connected locally so they can communicate
//! in a thread-safe way.
class RN_UdpConnectorImpl::LocalConnectionSharedState {
public:
    enum Status {
        STATUS_ACTIVE,
        STATUS_ENDED_GRACEFUL,
        STATUS_ENDED_ERROR
    };

    LocalConnectionSharedState(RN_UdpConnectorImpl& aConnector1, RN_UdpConnectorImpl& aConnector2)
        : _connector1{aConnector1}
        , _connector2{aConnector2} {}

    void putData(RN_UdpConnectorImpl& aSelf, std::vector<util::Packet>&& aPackets) {
        {
            std::lock_guard<decltype(_mutex)> lock{_mutex};
            auto&                             targetVector =
                (&aSelf == &_connector1) ? _packetsForConnector2 : _packetsForConnector1;
            if (targetVector.empty()) {
                targetVector = std::move(aPackets);
            } else {
                for (auto& packet : aPackets) {
                    targetVector.emplace_back(std::move(packet));
                }
            }
        }
        aPackets.clear();
    }

    //! Returns true if any data was received
    bool getData(RN_UdpConnectorImpl& aSelf) {
        std::vector<util::Packet> temp;
        {
            std::lock_guard<decltype(_mutex)> lock{_mutex};

            auto& sourceVector =
                (&aSelf == &_connector1) ? _packetsForConnector1 : _packetsForConnector2;
            std::swap(sourceVector, temp);
        }

        if (temp.empty()) {
            return false;
        }

        for (auto& packet : temp) {
            aSelf.receivedPacket(packet);
        }

        return true;
    }

    Status getStatus() const {
        std::lock_guard<decltype(_mutex)> lock{_mutex};
        return _status;
    }

    void setStatus(Status aNewStatus) {
        std::lock_guard<decltype(_mutex)> lock{_mutex};
        // Status values rise in order of severity,
        // so keep the highest value (worst one):
        if (aNewStatus > _status) {
            _status = aNewStatus;
        }
    }

private:
    RN_UdpConnectorImpl&      _connector1;
    RN_UdpConnectorImpl&      _connector2;
    std::vector<util::Packet> _packetsForConnector1;
    std::vector<util::Packet> _packetsForConnector2;
    mutable std::mutex        _mutex;
    Status                    _status = STATUS_ACTIVE;
};

namespace {
void HandleDataMessages(util::Packet&             receivedPacket,
                        RN_NodeInterface&         node,
                        RN_UdpConnectorImpl&      activeConnector,
                        NeverNull<util::Packet**> pointerToCurrentPacket) {
    *pointerToCurrentPacket = &receivedPacket;

    while (activeConnector.getStatus() != RN_ConnectorStatus::Disconnected &&
           !receivedPacket.endOfPacket()) {
        const auto handlerId = receivedPacket.extract<rn_detail::RN_HandlerId>();
        auto handlerFunc     = rn_detail::RN_GlobalHandlerMapper::getInstance().handlerWithId(handlerId);
        if (handlerFunc == nullptr) {
            throw RN_IllegalMessage{"Requested handler does not exist."};
        }
        // It spams too much; uncomment if you need to debug.
        // HG_LOG_HPDEB(LOG_ID, "Going to execute handler with ID {}.", handlerId);
        (*handlerFunc)(node);
    }

    *pointerToCurrentPacket = nullptr;
}
} // namespace

RN_UdpConnectorImpl::RN_UdpConnectorImpl(RN_SocketAdapter&                aSocket,
                                         const std::chrono::microseconds& aTimeoutLimit,
                                         const std::string&               aPassphrase,
                                         const RN_RetransmitPredicate&    aRetransmitPredicate,
                                         rn_detail::EventFactory          aEventFactory,
                                         PZInteger                        aMaxPacketSize)
    : _socket{aSocket}
    , _timeoutLimit{aTimeoutLimit}
    , _passphrase{aPassphrase}
    , _retransmitPredicate{aRetransmitPredicate}
    , _eventFactory{aEventFactory}
    , _maxPacketSize{aMaxPacketSize}
    , _status{RN_ConnectorStatus::Disconnected}
    , _sendBuffer{_maxPacketSize, _retransmitPredicate}
    , _recvBuffer{} {}

// MARK: Accepting

bool RN_UdpConnectorImpl::tryAccept(sf::IpAddress addr, std::uint16_t port, util::Packet& packet) {
    HG_VALIDATE_PRECONDITION(_status == RN_ConnectorStatus::Disconnected);

    const std::uint32_t packetKind         = packet.extractNoThrow<std::uint32_t>();
    const std::string   receivedPassphrase = packet.extractNoThrow<std::string>();
    if (!packet) {
        HG_LOG_WARN(LOG_ID,
                    "Connection attempt from {}:{} refused because packet kind and/or passphrase "
                    "couldn't be read from the received packet.",
                    addr.toString(),
                    port);
        return false;
    }

    if (packetKind == UDP_PACKET_KIND_HELLO && receivedPassphrase == _passphrase) {
        _remoteInfo = RN_RemoteInfo{addr, port};
        _status     = RN_ConnectorStatus::Accepting;

        _resetBuffers();
    } else {
        HG_LOG_WARN(LOG_ID,
                    "Connection attempt from {}:{} refused because packet kind and/or passphrase "
                    "of the initial packet were incorrect (packet kind: {}; passphrase: {}).",
                    addr.toString(),
                    port,
                    packetKind,
                    (packetKind == UDP_PACKET_KIND_HELLO) ? receivedPassphrase : "n/a");
        return false;
    }

    return true;
}

bool RN_UdpConnectorImpl::tryAcceptLocal(RN_UdpConnectorImpl& localPeer, const std::string& passphrase) {
    HG_VALIDATE_PRECONDITION(_status == RN_ConnectorStatus::Disconnected);

    if (passphrase == _passphrase) {
        _localSharedState           = std::make_shared<LocalConnectionSharedState>(SELF, localPeer);
        localPeer._localSharedState = _localSharedState;

        _remoteInfo = RN_RemoteInfo{sf::IpAddress::LocalHost, 0};
        _status     = RN_ConnectorStatus::Connected;
        _resetBuffers();

        _startSession();
        _eventFactory.createConnected();

        return true;
    }

    HG_LOG_WARN(LOG_ID,
                "Local connection was refused due to providing a wrong passphrase ({}).",
                passphrase);

    return false;
}

// MARK: Connecting

void RN_UdpConnectorImpl::connect(sf::IpAddress addr, std::uint16_t port) {
    HG_VALIDATE_PRECONDITION(_status == RN_ConnectorStatus::Disconnected);

    _remoteInfo = RN_RemoteInfo{addr, port};
    _status     = RN_ConnectorStatus::Connecting;

    _resetBuffers();
}

void RN_UdpConnectorImpl::connectLocal(RN_ServerInterface& server) {
    HG_VALIDATE_PRECONDITION(_status == RN_ConnectorStatus::Disconnected);

    auto* udpServer = dynamic_cast<RN_UdpServerImpl*>(&server);
    if (!udpServer) {
        HG_THROW_TRACED(TracedLogicError, 0, "Incompatible node types for local connection.");
    }

    const int clientIndex = udpServer->acceptLocalConnection(SELF, _passphrase);
    if (clientIndex < 0) {
        HG_THROW_TRACED(TracedLogicError, 0, "Local connection refused.");
    }

    HG_HARD_ASSERT(_localSharedState != nullptr);

    _remoteInfo = RN_RemoteInfo{sf::IpAddress::LocalHost, 0};
    _status     = RN_ConnectorStatus::Connected;

    _resetBuffers();

    _clientIndex = clientIndex;
    _eventFactory.createConnected();
}

// MARK: Receiving

void RN_UdpConnectorImpl::prepToReceive() {
    _newMeanLatency       = decltype(_newMeanLatency){0};
    _newLatencySampleSize = 0;
}

void RN_UdpConnectorImpl::receivedPacket(util::Packet& packet) {
    assert(_status != RN_ConnectorStatus::Disconnected);

    std::optional<TracedException> exception;
    try {
        const auto packetKind = packet.extract<std::uint32_t>();

        switch (packetKind) {
        case UDP_PACKET_KIND_HELLO:
            _processHelloPacket(packet);
            break;

        case UDP_PACKET_KIND_CONNECT:
            _processConnectPacket(packet);
            break;

        case UDP_PACKET_KIND_DISCONNECT:
            _processDisconnectPacket(packet);
            break;

        case UDP_PACKET_KIND_DATA:
            _processDataPacket(packet);
            break;

        case UDP_PACKET_KIND_DATA_MORE:
            _processDataMorePacket(packet);
            break;

        case UDP_PACKET_KIND_DATA_TAIL:
            _processDataTailPacket(packet);
            break;

        case UDP_PACKET_KIND_ACKS:
            _processAcksPacket(packet);
            break;

        default:
            HG_THROW_TRACED(InvalidDataError, 0, "Received packet of unknown kind ({}).", packetKind);
            break;
        }
    } catch (const InvalidDataError& ex) {
        exception = ex;
    } catch (const util::PacketReadError& ex) {
        exception = ex;
    }

    if (exception) {
        HG_LOG_ERROR(
            LOG_ID,
            "An error occurred while processing a received packet; resetting connector. Details: {}",
            exception->getFormattedDescription());

        _resetAll();
        if (_status == RN_ConnectorStatus::Connected) {
            if (_isConnectedLocally()) {
                _localSharedState->setStatus(LocalConnectionSharedState::STATUS_ENDED_ERROR);
            }
            _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Error,
                                             exception->getErrorMessage());
        } else {
            _eventFactory.createConnectAttemptFailed(RN_Event::ConnectAttemptFailed::Reason::Error);
        }
    }
}

void RN_UdpConnectorImpl::receivingFinished() {
    if (_newLatencySampleSize > 0) {
        _remoteInfo.meanLatency        = (_newMeanLatency / _newLatencySampleSize);
        _remoteInfo.optimisticLatency  = _newOptimisticLatency;
        _remoteInfo.pessimisticLatency = _newPessimisticLatency;
    }
}

RN_Telemetry RN_UdpConnectorImpl::sendWeakAcks() {
    assert(_status == RN_ConnectorStatus::Connected);

    RN_Telemetry telemetry;
    telemetry.uploadByteCount += _sendBuffer.sendWeakAcks([this](util::Packet& aPacket) {
        _socket.send(aPacket, _remoteInfo.ipAddress, _remoteInfo.port);
    });
    if (telemetry.uploadByteCount > 0) {
        telemetry.uploadByteCount += UDP_HEADER_BYTE_COUNT;
    }
    // TODO: should sendWeakAcks care about a packet limiter?

    return telemetry;
}

void RN_UdpConnectorImpl::handleDataMessages(RN_NodeInterface&         aNode,
                                             NeverNull<util::Packet**> aCurrentPacketPtr) {
    assert(_status != RN_ConnectorStatus::Disconnected);

    if (_isConnectedLocally()) {
        if (_localSharedState->getData(SELF)) {
            _remoteInfo.timeoutStopwatch.restart();
        }
    }

    try {
        util::Packet packet;
        while (_recvBuffer.takeNextReadyPacket(&packet)) {
            HandleDataMessages(packet, aNode, SELF, aCurrentPacketPtr);
            if (getStatus() == RN_ConnectorStatus::Disconnected) {
                break; // Data messages can cause a disconnect in rare circumstances
                       // (if the handler is explicitly programmed to do so)
            }
        }
    } catch (util::PacketReadError& ex) {
        if (_isConnectedLocally()) {
            _localSharedState->setStatus(LocalConnectionSharedState::STATUS_ENDED_ERROR);
        }
        _resetAll();
        _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Error, ex.getDescription());
    } catch (RN_IllegalMessage& ex) {
        _resetAll();
        _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Error, ex.what());
    }

    if (_isConnectedLocally()) {
        switch (_localSharedState->getStatus()) {
        case LocalConnectionSharedState::STATUS_ACTIVE:
            // All good, carry on
            break;

        case LocalConnectionSharedState::STATUS_ENDED_GRACEFUL:
            _resetAll();
            _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Graceful,
                                             "Remote terminated the connection.");
            break;

        case LocalConnectionSharedState::STATUS_ENDED_ERROR:
            _resetAll();
            _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Error,
                                             "Connection closed due to an error.");
            break;

        default:
            HG_UNREACHABLE();
        }
    }
}

void RN_UdpConnectorImpl::checkForTimeout() {
    assert(_status != RN_ConnectorStatus::Disconnected);

    if (_isConnectionTimedOut()) {
        _resetAll();

        if (_status == RN_ConnectorStatus::Accepting || _status == RN_ConnectorStatus::Connecting) {
            _eventFactory.createConnectAttemptFailed(RN_Event::ConnectAttemptFailed::Reason::TimedOut);
        } else {
            _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::TimedOut,
                                             "Connection timed out");
        }
    }
}

// MARK: Sending

void RN_UdpConnectorImpl::appendDataForSending(NeverNull<const void*> aData, PZInteger aDataByteCount) {
    _sendBuffer.appendDataForSending(aData, aDataByteCount);
}

RN_Telemetry RN_UdpConnectorImpl::sendData() {
    assert(_status != RN_ConnectorStatus::Disconnected);
    RN_Telemetry telemetry;

    switch (_status) {
    case RN_ConnectorStatus::Accepting: // Send CONNECT packets to the client, until a DATA packet is
                                        // received
        {
            util::Packet packet;
            packet << UDP_PACKET_KIND_CONNECT << _passphrase << _clientIndex.value();

            // Safe to ignore recoverable errors here - Disconnected doesn't happen with UDP and
            // NotReady is irrelevant because CONNECTs keep getting resent until acknowledged anyway
            _socket.send(packet, _remoteInfo.ipAddress, _remoteInfo.port);
            telemetry.uploadByteCount += stopz(packet.getDataSize() + UDP_HEADER_BYTE_COUNT);
        }
        break;

    case RN_ConnectorStatus::Connecting: // Send HELLO packets to the server, until a CONNECT packet is
                                         // received
        {
            util::Packet packet;
            packet << UDP_PACKET_KIND_HELLO << _passphrase;

            // Safe to ignore recoverable errors here - Disconnected doesn't happen with UDP and
            // NotReady is irrelevant because HELLOs keep getting resent until acknowledged anyway
            _socket.send(packet, _remoteInfo.ipAddress, _remoteInfo.port);
            telemetry.uploadByteCount += stopz(packet.getDataSize() + UDP_HEADER_BYTE_COUNT);
        }
        break;

    case RN_ConnectorStatus::Connected:
        if (!_isConnectedLocally()) {
            telemetry.uploadByteCount += _uploadAllData();
        } else {
            _transferAllDataToLocalPeer();
        }
        break;

    default:
        HG_UNREACHABLE();
        break;
    }

    return telemetry;
}

// MARK: Client index

void RN_UdpConnectorImpl::setClientIndex(std::optional<PZInteger> clientIndex) {
    _clientIndex = clientIndex;
}

std::optional<PZInteger> RN_UdpConnectorImpl::getClientIndex() const {
    return _clientIndex;
}

// MARK: Inherited from RN_ConnectorInterface

const RN_RemoteInfo& RN_UdpConnectorImpl::getRemoteInfo() const noexcept {
    return _remoteInfo;
}

RN_ConnectorStatus RN_UdpConnectorImpl::getStatus() const noexcept {
    return _status;
}

bool RN_UdpConnectorImpl::isConnected() const noexcept {
    return _status == RN_ConnectorStatus::Connected;
}

bool RN_UdpConnectorImpl::isDisconnected() const noexcept {
    return _status == RN_ConnectorStatus::Disconnected;
}

void RN_UdpConnectorImpl::disconnect(bool aNotfiyRemote, const std::string& aMessage) {
    HG_VALIDATE_PRECONDITION(_status != RN_ConnectorStatus::Disconnected);

    if (aNotfiyRemote && _status == RN_ConnectorStatus::Connected) {
        if (!_isConnectedLocally()) {
            util::Packet packet;
            packet << UDP_PACKET_KIND_DISCONNECT << aMessage;

            // Ignore all recoverable errors - The connector is getting disconnected anyway...
            _socket.send(packet, _remoteInfo.ipAddress, _remoteInfo.port);
        }
    }

    _resetAll();

    _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Graceful, aMessage);
}

bool RN_UdpConnectorImpl::isConnectedLocally() const noexcept {
    return _isConnectedLocally();
}

PZInteger RN_UdpConnectorImpl::getSendBufferSize() const {
    return _sendBuffer.getLength();
}

PZInteger RN_UdpConnectorImpl::getRecvBufferSize() const {
    return _recvBuffer.getLength();
}

///////////////////////////////////////////////////////////////////////////
// MARK: PRIVATE METHODS                                                 //
///////////////////////////////////////////////////////////////////////////

bool RN_UdpConnectorImpl::_isConnectedLocally() const noexcept {
    return (_localSharedState != nullptr);
}

void RN_UdpConnectorImpl::_resetBuffers() {
    _sendBuffer.reset();
    _recvBuffer.reset();
}

void RN_UdpConnectorImpl::_resetAll() {
    _resetBuffers();

    _remoteInfo = RN_RemoteInfo{};
    _status     = RN_ConnectorStatus::Disconnected;
    _clientIndex.reset();

    if (_isConnectedLocally()) {
        _localSharedState->setStatus(LocalConnectionSharedState::STATUS_ENDED_GRACEFUL);
        _localSharedState.reset();
    }
}

bool RN_UdpConnectorImpl::_isConnectionTimedOut() const {
    if (_timeoutLimit <= std::chrono::microseconds{0}) {
        return false;
    }
    if ((_remoteInfo.timeoutStopwatch.getElapsedTime() >= _timeoutLimit) && !_isConnectedLocally()) {
        return true;
    }
    return false;
}

PZInteger RN_UdpConnectorImpl::_uploadAllData() {
    // TODO: propagate socket status upwards
    // TODO: better handling of packet limiter

    PZInteger  packetLimiter = 10;
    const auto result =
        _sendBuffer.sendData(&packetLimiter,
                             _remoteInfo.meanLatency,
                             [this](util::Packet& aPacket) -> RN_SocketAdapter::Status {
                                 return _socket.send(aPacket, _remoteInfo.ipAddress, _remoteInfo.port);
                             });

    switch (result.socketStatus) {
    case RN_SocketAdapter::Status::OK:
        break;

    case RN_SocketAdapter::Status::NotReady:
        break;

    case RN_SocketAdapter::Status::Disconnected:
        // Normally we wouldn't expect to get this status from UDP sockets. However,
        // it case it does somehow happen, treat it as the remote gracefully terminating
        // the connection.
        _eventFactory.createDisconnected(RN_Event::Disconnected::Reason::Graceful,
                                         "Remote terminated the connection");
        _resetAll();
        break;

    default:
        HG_UNREACHABLE("Invalid value for RN_SocketAdapter::Status ({}).", (int)result.socketStatus);
    }

    return result.uploadedByteCount;
}

void RN_UdpConnectorImpl::_transferAllDataToLocalPeer() {
    assert(_isConnectedLocally());
    _localSharedState->putData(SELF, _sendBuffer.exportPackets());
}

void RN_UdpConnectorImpl::_prepareAck(std::uint32_t ordinal) {
    if (_isConnectedLocally()) {
        // No acks needed in local connections.
        return;
    }
    _sendBuffer.appendAckForSending(ordinal);
}

void RN_UdpConnectorImpl::_receivedAck(std::uint32_t ordinal, bool strong) {
    const auto result = _sendBuffer.ackReceived(ordinal, strong);

    if (result.isSignificant) {
        _remoteInfo.timeoutStopwatch.restart();

        _newMeanLatency += result.timeToAck;
        if (_newLatencySampleSize == 0) {
            _newOptimisticLatency  = result.timeToAck;
            _newPessimisticLatency = result.timeToAck;
        } else {
            _newOptimisticLatency  = std::min(_newOptimisticLatency, result.timeToAck);
            _newPessimisticLatency = std::max(_newPessimisticLatency, result.timeToAck);
        }
        _newLatencySampleSize += 1;
    }
}

void RN_UdpConnectorImpl::_startSession() {
    _status = RN_ConnectorStatus::Connected;
    _remoteInfo.timeoutStopwatch.restart();
}

void RN_UdpConnectorImpl::_saveDataPacket(util::Packet& packet, std::uint32_t packetType) {
    const std::uint32_t packetOrdinal = packet.extract<std::uint32_t>();
    _prepareAck(packetOrdinal);

    const auto acks = _recvBuffer.storeDataPacket(std::move(packet), packetOrdinal, packetType);
    for (const auto ack : acks) {
        _receivedAck(ack, true);
    }
}

// MARK: Packet processing

void RN_UdpConnectorImpl::_processHelloPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received HELLO packet (status: Connecting).");

    case RN_ConnectorStatus::Accepting:
        NO_OP();
        break;

    case RN_ConnectorStatus::Connected:
        HG_LOG_WARN(LOG_ID, "Received a HELLO packet but the connector is already connected; ignoring.");
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processConnectPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        {
            auto            receivedPassphrase  = packet.extract<std::string>();
            const PZInteger receivedClientIndex = packet.extract<PZInteger>();
            if (receivedPassphrase == _passphrase) {
                // Client connected to server
                _clientIndex = receivedClientIndex;
                _startSession();
                _eventFactory.createConnected();
            } else {
                _resetAll();
                _eventFactory.createBadPassphrase(std::move(receivedPassphrase));
            }
        }
        break;

    case RN_ConnectorStatus::Accepting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received CONNECT packet (status: Accepting).");

    case RN_ConnectorStatus::Connected:
        HG_LOG_WARN(LOG_ID,
                    "Received a CONNECT packet but the connector is already connected; ignoring.");
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processDisconnectPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
    case RN_ConnectorStatus::Accepting:
    case RN_ConnectorStatus::Connected:
        {
            auto message = packet.extractNoThrow<std::string>();
            if (!packet) {
                message = "No additional details available.";
            }
            _resetAll();
            _eventFactory.createDisconnected(
                RN_Event::Disconnected::Reason::Graceful,
                fmt::format(FMT_STRING("Remote terminated the connection: {}"), message));
        }
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processDataPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received DATA packet (status: Connecting).");

    case RN_ConnectorStatus::Accepting:
        _startSession(); // New connection confirmed
        _eventFactory.createConnected();
        SWITCH_FALLTHROUGH;

    case RN_ConnectorStatus::Connected:
        _saveDataPacket(packet, UDP_PACKET_KIND_DATA);
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processDataMorePacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received DATA_MORE packet (status: Connecting).");

    case RN_ConnectorStatus::Accepting:
        _startSession(); // New connection confirmed
        _eventFactory.createConnected();
        SWITCH_FALLTHROUGH;

    case RN_ConnectorStatus::Connected:
        _saveDataPacket(packet, UDP_PACKET_KIND_DATA_MORE);
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processDataTailPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received DATA_TAIL packet (status: Connecting).");

    case RN_ConnectorStatus::Accepting:
        _startSession(); // New connection confirmed
        _eventFactory.createConnected();
        SWITCH_FALLTHROUGH;

    case RN_ConnectorStatus::Connected:
        _saveDataPacket(packet, UDP_PACKET_KIND_DATA_TAIL);
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

void RN_UdpConnectorImpl::_processAcksPacket(util::Packet& packet) {
    switch (_status) {
    case RN_ConnectorStatus::Connecting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received ACKS packet (status: Connecting).");

    case RN_ConnectorStatus::Accepting:
        HG_THROW_TRACED(InvalidDataError, 0, "Received ACKS packet (status: Accepting).");

    case RN_ConnectorStatus::Connected:
        while (!packet.endOfPacket()) {
            const auto ackOrd = packet.extract<PacketOrdinal>();
            _receivedAck(ackOrd, false);
        }
        break;

    default:
        HG_UNREACHABLE("Invalid value for _status ({}).", (int)_status);
        break;
    }
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
