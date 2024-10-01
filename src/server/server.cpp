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
            continue;
        }

        if (!std::holds_alternative<ClientChatMessage>(msg->payload)) {
            spdlog::info("received message type other than Chat Message, skipping as not implemented");
            continue;
        }

        auto chatMsg = std::get<ClientChatMessage>(msg->payload);
        
        // TODO: later handle this as part of a connection request
        if (!d_clients.contains(msg->senderId)) {
            d_clients.insert(msg->senderId);
            spdlog::info("New client connected: {}", msg->senderId);
            broadcastNewConnection(msg->senderId);
        }

        spdlog::info("Received message: [{}] {}", msg->senderId, chatMsg.message);

        ServerChatMessage serverMsg{msg->senderId, chatMsg.message}; 
        broadcastMessage(serverMsg);
    }
}

void Server::broadcastNewConnection(const std::string& id) {
    ServerChatMessage serverMsg{"ALERT", "New client connected: " + id};
    broadcastMessage(serverMsg);
}

std::optional<ClientBaseMessage> Server::receiveMessage() {
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
    std::string data = std::string(static_cast<char*>(msg.data()), msg.size());

    auto clientBaseMsg = deserialize_clientbasemsg(data);
    if (!clientBaseMsg.has_value()) {
        spdlog::warn("Failed to deserialize client base message in Server::receiveMessage");
        return std::nullopt;
    }
    else if (clientBaseMsg->senderId != idStr) {
        spdlog::warn("Sender ID does not match ID in message");
        return std::nullopt;
    }

    return clientBaseMsg;
}

void Server::broadcastMessage(const ServerChatMessage& message) {
    // may be a better spot elsewhere for serializing
    ServerBaseMessage baseMessage{message};
    auto serialized = serialize_serverbasemsg(baseMessage);
    if (!serialized.has_value()) {
        spdlog::warn("Failed to serialize message in Server::broadcastMessage");
        return;
    }

    for (const auto& client : d_clients) {
        if (client == message.senderId) {
            continue;
        }
        zmq::message_t id(client);
        zmq::message_t msg(*serialized);
        routerSocket.send(id, zmq::send_flags::sndmore);
        routerSocket.send(msg, zmq::send_flags::none);
    }
}
