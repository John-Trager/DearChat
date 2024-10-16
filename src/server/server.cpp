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

        if (std::holds_alternative<ClientChatMessage>(msg->payload)) {
            handleClientChatMessage(*msg);
        } else if (std::holds_alternative<ClientConnectionRequest>(msg->payload)) {
            handleClientConnectionRequest(*msg);
        } else if (std::holds_alternative<ClientCreateRoomRequest>(msg->payload)) {
            handleClientCreateRoomRequest(*msg);
        } else {
            spdlog::warn("Received unknown message type");
        }
    }
}

void Server::createRoom(const std::string& room_id) {
    if (validRoomId(room_id)) {
        spdlog::error("Attempted to create room that already exists: {}", room_id);
        return;
    }

    d_rooms[room_id] = Room{};
}

// BUSINESS LOGIC FUNCTIONS
void Server::handleClientChatMessage(const ClientBaseMessage& msg) {
    auto chatMessage = std::get<ClientChatMessage>(msg.payload).message;
    auto senderId = msg.senderId;

    if (!isClientValid(senderId)) {
        // TODO: also log the state of the client and client data
        spdlog::warn("Received ClientChatMessage from invalid client: {}", senderId);
        return;
    }

    spdlog::info("Received message: [{}] {}", senderId, chatMessage);

    // TODO: add timestamp into the message
    ServerChatMessage serverMsg{senderId, chatMessage}; 
    broadcastMessage(serverMsg);
}

void Server::handleClientConnectionRequest(const ClientBaseMessage& msg) {
    auto roomId = std::get<ClientConnectionRequest>(msg.payload).roomId;
    auto senderId = msg.senderId;
    
    // if we have a new client, initialize them
    if (!d_clients.contains(senderId)) {
        spdlog::info("New client created, name: {}", senderId);       
        d_clients.insert(senderId);
        d_clientData[senderId] = Client{};
    }

    if (validRoomId(roomId) && d_clientData[senderId].room != roomId) {
        addClientToRoom(senderId, roomId);
        spdlog::info("Client {} connected to room: {}", senderId, roomId);
        sendConnectionResponse(senderId, true, std::nullopt);
        //TODO: should broadcast to all clients in the room that a new client has connected

    } else if (!validRoomId(roomId)) {
        spdlog::warn("Client {} attempted to connect to invalid room {}", senderId, roomId);
        sendConnectionResponse(senderId, false, "Invalid room ID");
    } else {
        // TODO: bug herem if client closes app and then re-opens they will try to connect to the same room twice
        // so need some exit message or heartbeat to remove client from room
        spdlog::warn("Client {} attempted to connect to room {} that they are already in", senderId, roomId);
        sendConnectionResponse(senderId, false, "Already in room");
    } 
}

void Server::handleClientCreateRoomRequest(const ClientBaseMessage& msg) {
    auto roomId = std::get<ClientCreateRoomRequest>(msg.payload).roomId;
    auto senderId = msg.senderId;


    if (validRoomId(roomId)) {
        spdlog::warn("Client {} attempted to create room that already exists: {}", senderId, roomId);
        sendCreateRoomResponse(senderId, false, "Room already exists");
        return;
    }
   
    // if we have a new client, initialize them
    if (!d_clients.contains(senderId)) {
        spdlog::info("New client created, name: {}", senderId);       
        d_clients.insert(senderId);
        d_clientData[senderId] = Client{};
    }

    createRoom(roomId);
    addClientToRoom(senderId, roomId);
    spdlog::info("Client {} created and connected to new room: {}", senderId, roomId);
    sendCreateRoomResponse(senderId, true, std::nullopt);
}

// NETWORKING FUNCTIONS

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

    auto senderRoomId = d_clientData[message.senderId].room;
    auto& room = d_rooms[senderRoomId];

    for (const auto& client : room.clients) {
        if (client == message.senderId) {
            continue;
        }
        zmq::message_t id(client);
        zmq::message_t msg(*serialized);
        routerSocket.send(id, zmq::send_flags::sndmore);
        routerSocket.send(msg, zmq::send_flags::none);
    }

    // save message to room history
    room.history.push_back(message);
}

void Server::sendConnectionResponse(const std::string& id, bool accepted, const std::optional<std::string>& reason) {
    ServerConnectionResponse response{accepted, reason};
    ServerBaseMessage baseMessage{response};
    auto serialized = serialize_serverbasemsg(baseMessage);
    if (!serialized.has_value()) {
        spdlog::error("Failed to serialize message in Server::sendConnectionResponse");
        return;
    }

    zmq::message_t idMsg(id);
    zmq::message_t msg(*serialized);
    routerSocket.send(idMsg, zmq::send_flags::sndmore);
    routerSocket.send(msg, zmq::send_flags::none);
}

void Server::sendCreateRoomResponse(const std::string& id, bool accepted, const std::optional<std::string>& reason) {
    ServerCreateRoomResponse response{accepted, reason};
    ServerBaseMessage baseMessage{response};
    auto serialized = serialize_serverbasemsg(baseMessage);
    if (!serialized.has_value()) {
        spdlog::error("Failed to serialize message in Server::sendCreateRoomResponse");
        return;
    }

    zmq::message_t idMsg(id);
    zmq::message_t msg(*serialized);
    routerSocket.send(idMsg, zmq::send_flags::sndmore);
    routerSocket.send(msg, zmq::send_flags::none);
}