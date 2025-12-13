#include "server.hpp"

#include "fmt/format.h"
#include "s_logger.hpp"
#include <iostream>
#include <unistd.h>

Server::Server(int port = 7777) : m_router(port) {
    // std::signal(SIGINT, stop);
    // std::signal(SIGHUP, stop);
    // std::signal(SIGTERM, stop);
}

void Server::run() {

    m_router.use_ssl("/home/piotr/Projects/ChatTerminal/cert/server.pem",
                     "/home/piotr/Projects/ChatTerminal/cert/server.key");
    m_router.on_packet<Proto::LoginRequest>([this](int fd, Proto::LoginRequest packet) {
        Logger::server_message(fmt::format("A login request from {}", packet.username));
        Proto::Message msg;
        msg.message  = "request";
        msg.username = "dupa";
        broadcast_message(msg);
    });
    m_router.on_packet<Proto::Message>(
        [this](int fd, Proto::Message packet) { broadcast_message(packet); });

    Logger::server_message(fmt::format("Server is running on port {}", 7777));
    m_router.run();
}

void Server::broadcast_message(Proto::Message &msg) {
    std::shared_ptr<Proto::Bytes> s = std::make_shared<Proto::Bytes>(msg.serialize());
    for (const auto &[key, client] : m_router.clients) {
        client->async_write(s);
    }
}