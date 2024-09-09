#pragma once

#include <string>
#include <zmq.hpp>
#include <optional>
#include <unordered_set>

struct Message {
    std::string id;
    std::string message;
};

class Server{

public:
    Server(const std::string& address);

    void run();

    private:
    zmq::context_t context;
    zmq::socket_t routerSocket;

    std::optional<Message> receiveMessage();
    void broadcastMessage(const Message& message);

    std::unordered_set<std::string> d_clients;
};