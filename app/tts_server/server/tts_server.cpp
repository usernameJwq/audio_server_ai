#include "tts_server.h"

#include <string>

#include "logs.h"

Server::Server(const int& port) : port_(port) {}

Server::~Server() {}

void Server::run() {
    std::string host = "0.0.0.0";

    LOG::info("serevr listen success, port is {}", std::to_string(port_));

    bool ret = server_.listen(host, port_);
    if (!ret) {
        LOG::error("serevr listen port {} failed", std::to_string(port_));
    }
}