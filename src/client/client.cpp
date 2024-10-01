#include "client.h"
#include "messaging.h"

#include <zmq.hpp>
#include <zmq.h>
#include <vector>
#include <iostream>

const std::string Client::s_inprocAddr = "inproc://sender";

Client::Client(const std::string& address, const std::string& id) 
: d_clientId(id)
, d_serverAddr(address)
, d_context(1)
, d_sender(d_context, ZMQ_PAIR)
, d_agentDone(false)
, console([this](const std::string& message) { send(message); })
{
    d_sender.bind(s_inprocAddr);
    d_agentThread = std::thread(&Client::agent, this);
    ClientConnectionRequest connectionRequest;
}

Client::~Client() {
    d_agentDone = true;
    
    if (d_agentThread.joinable()) {
        spdlog::debug("Joining agent thread");   
        d_agentThread.join();
    } else {
        spdlog::warn("Agent thread not joinable");
    }
    d_sender.close();
    d_context.close();
}

void Client::send(const std::string& message) {
    // dont allow empty messages to be sent
    if (message.empty()) {
       return;
    }

   /*
    zmq::message_t msg_type_t("SEND");
    auto res = d_sender.send(msg_type_t, zmq::send_flags::sndmore);
    if (!res.has_value()) {
        spdlog::warn("Failed to send message type on sender");
    }
   */

    ClientChatMessage chatMessage{message};
    ClientBaseMessage baseMessage{d_clientId, chatMessage};
    auto serialized = serialize_clientbasemsg(baseMessage);

    if (!serialized.has_value()) {
        spdlog::warn("Failed to serialize message in Client::send");
        return;
    }

    zmq::message_t msg_t(*serialized);
    auto res = d_sender.send(msg_t, zmq::send_flags::none);
    if (!res.has_value()) {
        spdlog::warn("Failed to send message on sender");
    }
}

void Client::agent() {
    spdlog::debug("Agent thread started");
    zmq::socket_t dealer(d_context, ZMQ_DEALER);
    dealer.set(zmq::sockopt::routing_id, d_clientId);
    dealer.connect(d_serverAddr);

    zmq::socket_t forwarder(d_context, ZMQ_PAIR);
    forwarder.set(zmq::sockopt::linger,0);
    forwarder.connect(s_inprocAddr);

    zmq::poller_t<> poller;
    poller.add(forwarder, zmq::event_flags::pollin); 
    poller.add(dealer, zmq::event_flags::pollin);

    std::vector<zmq::poller_event<>> events(2); // 2 sockets

    while (true) {
        // Poll for events
        auto n = poller.wait_all(events, std::chrono::milliseconds(3000));
        if (d_agentDone) {   
            break;
        } 
        // poller timeout event (or maybe error?)
        if (n == 0) {
            continue;
        }

        // Process events
        for (const auto& event : events) {
            if (event.socket == forwarder) {
                // forwarder has message
                zmq::message_t message;
                auto recv_res = forwarder.recv(message, zmq::recv_flags::none);
                if (!recv_res.has_value()) {
                    spdlog::warn("Failed to receive message on forwarder");
                    continue;
                }
                auto send_res = dealer.send(message, zmq::send_flags::none);
                if (!send_res.has_value()) {
                    spdlog::warn("Failed to send message on dealer (from forwarder)");
                    continue;
                }
            } else if (event.socket == dealer) {    
                // dealer has message
                zmq::message_t message;
                auto res = dealer.recv(message, zmq::recv_flags::none);
                if (!res.has_value()) {
                    spdlog::warn("Failed to receive message on dealer");
                    continue;
                }

                auto baseMessage = deserialize_serverbasemsg(message.to_string());
                if (!baseMessage.has_value()) {
                    spdlog::warn("Failed to deserialize server base message in Client::agent");
                    continue;
                }

                auto& payload = baseMessage->payload;
                if (std::holds_alternative<ServerChatMessage>(payload)) {
                    auto& message = std::get<ServerChatMessage>(payload);
                    spdlog::debug("Received message from server: {}", message.message);

                    std::string messageLine = "[" + message.senderId + "] " + message.message;

                    console.AddLog(messageLine);
                } else if (std::holds_alternative<ServerConnectionResponse>(payload)) {
                    auto& message = std::get<ServerConnectionResponse>(payload);
                } else {
                    spdlog::warn("Received unknown message type from server");
                }

            } // end if
        } // end for

    } // end while
    // cleanup
    dealer.close();
    forwarder.close();
}