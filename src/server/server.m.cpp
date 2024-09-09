#include "server.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <string>
#include <zmq.hpp>

int main(int argc, const char *argv[]){

    // set the log level to debug (globally)
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("server.m is running");

    Server server("tcp://*:8888");

    server.run();

    return 0;
}
