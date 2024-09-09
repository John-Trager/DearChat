#pragma once

#include "spdlog/spdlog.h"
#include "console.h"

#include <string>
// enable drafts for zmq::poller_t
#define ZMQ_BUILD_DRAFT_API
#include <zmq.hpp>
#include <thread>


class Client {

public:
    Client(const std::string& address, const std::string& id);
    
    ~Client();

    void send(const std::string& message);
    
    void agent();
    
    
    // public members
    Console console;
    
    private:

    // network stuff
    zmq::context_t d_context; 
    zmq::socket_t d_sender;
    static const std::string s_inprocAddr;

    // threads (1. for listening)
    std::thread d_agentThread;
    std::atomic_bool d_agentDone;

    std::string d_clientId;
    const std::string d_serverAddr;

};