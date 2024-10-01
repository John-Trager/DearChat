#pragma once

#include "messaging.h"

#include <string>
#include <zmq.hpp>
#include <optional>
#include <unordered_set>

class Server{

public:
    Server(const std::string& address);

    void run();

    private:
    zmq::context_t context;
    zmq::socket_t routerSocket;


    void broadcastNewConnection(const std::string& id);

    std::optional<ClientBaseMessage> receiveMessage();
    void broadcastMessage(const ServerChatMessage& message);

    std::unordered_set<std::string> d_clients;
};