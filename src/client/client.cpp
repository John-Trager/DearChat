#include "client.h"

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
    /*
    [message type] 1st packet TODO: use multipart message
    [message] 2nd packet
    */

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

    zmq::message_t msg_t(message);
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

                spdlog::debug("Received message from server: {}", message.to_string());

                std::string messageLine = "[Anon] " + message.to_string();

                console.AddLog(messageLine);
            } // end if
        } // end for

    } // end while
    // cleanup
    dealer.close();
    forwarder.close();
}