#include "client.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <string>
#include <zmq.hpp>

/*
Allows client to send messages to the server

but only see replies through spdlog debug messages
*/

int main(int argc, const char *argv[]){

    // set the log level to debug (globally)
    //spdlog::set_level(spdlog::level::debug);

    spdlog::info("client.m is running");

    // take in user string for client id
    std::string client_id;
    std::cout << "Enter client id: ";
    std::cin >> client_id;

    Client client("tcp://localhost:8888", client_id);

    while (true) {
        std::string message;
        std::cout << "Enter message: ";
        std::getline(std::cin, message);
        client.send(message);
    }

    return 0;
}
