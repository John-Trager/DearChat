#pragma once

#include "messaging.h"

#include <string>
#include <zmq.hpp>
#include <optional>
#include <unordered_set>
#include <unordered_map>

// a struct to hold client data
struct Client {
    std::string room;
};

struct Room {
    std::unordered_set<std::string> clients;
    std::vector<ServerChatMessage> history;
};

class Server{

public:
    Server(const std::string& address);

    void run();

    void createRoom(const std::string& room_id);

    private:
    zmq::context_t context;
    zmq::socket_t routerSocket;
    // holds the all the client_ids
    std::unordered_set<std::string> d_clients;
    std::unordered_map<std::string, Client> d_clientData;

    std::unordered_map<std::string, Room> d_rooms;

    // BUSINESS LOGIC FUNCTIONS

    void handleClientChatMessage(const ClientBaseMessage& message);

    void handleClientConnectionRequest(const ClientBaseMessage& message);

    void handleClientCreateRoomRequest(const ClientBaseMessage& message);

    // NETWORKING FUNCTIONS

    void broadcastNewConnection(const std::string& id);
    void sendConnectionResponse(const std::string& id, bool accepted, const std::optional<std::string>& reason);
    void sendCreateRoomResponse(const std::string& id, bool accepted, const std::optional<std::string>& reason);

    std::optional<ClientBaseMessage> receiveMessage();
    void broadcastMessage(const ServerChatMessage& message);

    // INLINE FUNCTIONS

    // checks client exists in d_clients and d_clientData and is in a valid room
    bool isClientValid(const std::string& client_id);

    bool validRoomId(const std::string& room_id);
    
    bool isClientInRoom(const std::string& client_id, const std::string& room_id);

    void addClientToRoom(const std::string& client_id, const std::string& room_id);

    void removeClientFromRoom(const std::string& client_id, const std::string& room_id);
};

inline
bool Server::isClientValid(const std::string& client_id) {
    return d_clients.contains(client_id) && d_clientData.contains(client_id) && 
           validRoomId(d_clientData[client_id].room);
}

inline
bool Server::validRoomId(const std::string& room_id) {
    return d_rooms.contains(room_id);
}

inline
bool Server::isClientInRoom(const std::string& client_id, const std::string& room_id) {
    if (!d_rooms.contains(room_id)) {
        return false;
    }
    return d_rooms[room_id].clients.contains(client_id);
}

inline 
void Server::addClientToRoom(const std::string& client_id, const std::string& room_id) {
    // remove client from room if already in one
    if (d_clientData[client_id].room != "") {
        removeClientFromRoom(client_id, d_clientData[client_id].room);
    }

    d_clientData[client_id].room = room_id;
    d_rooms[room_id].clients.insert(client_id);
}

inline
void Server::removeClientFromRoom(const std::string& client_id, const std::string& room_id) {
    d_clientData[client_id].room = "";
    d_rooms[room_id].clients.erase(client_id);
}