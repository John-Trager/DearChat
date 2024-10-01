/*

Types of Messages:

Base Client Message:
- sender ID
- payload (variant)

Messages Clients can send:
1. Connection Request
technically empty

2. Chat Message
-  message

Messages Server can send:

Base Server Message:
- payload (variant)

1. Connection Response
- bool (accepted or not)
- optional reason message

2. Chat Message
- sender ID
- message
*/

#pragma once

#include <string>
#include <optional>
#include <variant>

#include "zpp_bits.h"

// --- Client Messages ---

struct ClientChatMessage {
    std::string message;
};

class ClientConnectionRequest { 
    // empty
};

struct ClientBaseMessage {
    // 2 members to serialize
    using serialize = zpp::bits::members<2>;
    
    std::string senderId;
    std::variant<ClientConnectionRequest, ClientChatMessage> payload;
};


// --- server Messages ---

struct ServerChatMessage {
    std::string senderId;
    std::string message;
};

struct ServerConnectionResponse {
    // 2 members to serialize
    using serialize = zpp::bits::members<2>;

    bool accepted;
    std::optional<std::string> reason;
};

struct ServerBaseMessage {
    std::variant<ServerConnectionResponse, ServerChatMessage> payload;
};

// --- Serialization/Deserialization Of Base Messages ---
// DO NOT CHANGE THE BELOW CODE

// - ClientBaseMessage -
inline
std::optional<std::string> serialize_clientbasemsg(const ClientBaseMessage& message) {
    std::string data;
    
    auto out = zpp::bits::out(data);
    auto res = out(message);
    if (failure(res)) {
        return std::nullopt;
    } 
    
    return data;
}

inline
std::optional<ClientBaseMessage> deserialize_clientbasemsg(const std::string& data) {
    auto in = zpp::bits::in(data);
    
    ClientBaseMessage message;
    auto res = in(message);
    if (failure(res)) {
        return std::nullopt;
    }
    
    return message;
}

// - ServerBaseMessage -
inline
std::optional<std::string> serialize_serverbasemsg(const ServerBaseMessage& message) {
    std::string data;
    
    auto out = zpp::bits::out(data);
    auto res = out(message);
    if (failure(res)) {
        return std::nullopt;
    } 
    
    return data;
}

inline
std::optional<ServerBaseMessage> deserialize_serverbasemsg(const std::string& data) {
    auto in = zpp::bits::in(data);
    
    ServerBaseMessage message;
    auto res = in(message);
    if (failure(res)) {
        return std::nullopt;
    }
    
    return message;
}