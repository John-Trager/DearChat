#include "server.h"
#include "spdlog/spdlog.h"

Server::Server(const std::string& address) 
: context(1)
, routerSocket(context, ZMQ_ROUTER)
{
    routerSocket.bind(address);
}


void Server::run() {
    while (true) {
        auto msg = receiveMessage();
        if (!msg.has_value()) {
            spdlog::warn("Failed to receive message");
            continue;
        }
        
        if (!d_clients.contains(msg->id)) {
            d_clients.insert(msg->id);
            spdlog::info("New client connected: {}", msg->id);
        }

        spdlog::info("Received message: [{}] {}", msg->id, msg->message);
        
        broadcastMessage(*msg);
    }
}

std::optional<Message> Server::receiveMessage() {
    zmq::message_t id;
    zmq::message_t msg;
    
    auto res = routerSocket.recv(id, zmq::recv_flags::none);
    if (!res.has_value()) {
        return std::nullopt;
    }

    res = routerSocket.recv(msg, zmq::recv_flags::none);
    if (!res.has_value()) {
        return std::nullopt;
    }

    std::string idStr = std::string(static_cast<char*>(id.data()), id.size());
    std::string msgStr = std::string(static_cast<char*>(msg.data()), msg.size());
    return Message{idStr, msgStr};
}

void Server::broadcastMessage(const Message& message) {
    for (const auto& client : d_clients) {
        if (client == message.id) {
            continue;
        }
        zmq::message_t id(client);
        zmq::message_t msg(message.message);
        routerSocket.send(id, zmq::send_flags::sndmore);
        routerSocket.send(msg, zmq::send_flags::none);
    }
}
