#ifndef NETWORKING_MANAGER_HPP
#define NETWORKING_MANAGER_HPP

#include <Hobgoblin/RigelNet.hpp>
#include <Hobgoblin/RigelNet_Macros.hpp>

#include <optional>

#include "Game_object_framework.hpp"

using namespace hg::rn;

class NetworkingManager : public GOF_StateObject {
public:
    using ServerType = RN_UdpServer;
    using ClientType = RN_UdpClient;

    NetworkingManager(QAO_Runtime* runtime, bool isServer);

    bool isServer() const noexcept;
    RN_Node& getNode();
    ServerType& getServer();
    ClientType& getClient();

    class EventListener {
    public:
        virtual void onNetworkingEvent(const RN_Event& event_) = 0;
    };

protected:
    void eventPreUpdate() override;
    // void eventUpdate() override;
    void eventPostUpdate() override;

private:
    std::variant<ServerType, ClientType> _node;
    bool _isServer;

    void handleEvents();
};

#endif // !NETWORKING_MANAGER_HPP
