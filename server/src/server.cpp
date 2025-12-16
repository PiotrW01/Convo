#include "server.hpp"

#include "bcrypt/BCrypt.hpp"
#include "fmt/format.h"
#include "s_logger.hpp"
#include <iostream>
#include <openssl/evp.h>
#include <unistd.h>

Server::Server(ServerConfig cfg) : m_config(cfg), m_router(cfg.port) {
    // std::signal(SIGINT, stop);
    // std::signal(SIGHUP, stop);
    // std::signal(SIGTERM, stop);
}

void Server::run() {
    sql::Driver *driver = sql::mariadb::get_driver_instance();
    m_db                = std::unique_ptr<sql::Connection>(
        driver->connect(m_config.db_address, m_config.db_user, m_config.db_password));
    m_db->setSchema(m_config.db_schema);

    m_router.use_ssl(m_config.cert_pem_path, m_config.cert_key_path);
    m_router.on_packet<Proto::Login>(
        [this](std::shared_ptr<Proto::Connection> conn, Proto::Login &packet) {
            std::string stored = "$2a$12$EY9ePLW9EhYGPbMB09icfuY4wZ/5gJunAg3znsdQfZnrSzG3b5Fwa";

            auto hash = BCrypt::validatePassword(packet.username, stored);
            Logger::server_message(fmt::format("A login request from {}", hash));
            Proto::Login req;
            req.username = packet.username;
            auto s       = std::make_shared<Proto::Bytes>(req.serialize());
            conn->async_write(s);
        });
    m_router.on_packet<Proto::Register>(
        [this](std::shared_ptr<Proto::Connection> conn, Proto::Register &packet) {
            auto stmt(m_db->prepareStatement("select count(*) from accounts where username = ?"));
            stmt->setString(1, packet.username);
            auto res(stmt->executeQuery());
            if (res->next()) {
                int count = res->getUInt(1);
                if (count == 0) {
                    auto stmt2(m_db->prepareStatement(
                        "insert into accounts (username, password_hash) values (?, 'passa')"));
                    stmt2->setString(1, packet.username);
                    auto r = stmt2->executeUpdate();
                    std::cout << r << std::endl;
                }
            }

            while (res->next()) {
                std::cout << fmt::format("{} {} {} {}", res->getInt(1), res->getString(2).c_str(),
                                         res->getString(3).c_str(), res->getString(4).c_str())
                          << std::endl;
            }

            Logger::server_message(fmt::format("A login request from {}", packet.username));
            Proto::Login req;
            req.username = packet.username;
            auto s       = std::make_shared<Proto::Bytes>(req.serialize());
            conn->async_write(s);
        });
    m_router.on_packet<Proto::Message>(
        [this](std::shared_ptr<Proto::Connection> conn, Proto::Message &packet) {
            broadcast_message(packet);
        });

    Logger::server_message(fmt::format("Server is running on port {}", 7777));
    m_router.run();
}

void Server::broadcast_message(Proto::Message &msg) {
    std::shared_ptr<Proto::Bytes> s = std::make_shared<Proto::Bytes>(msg.serialize());
    for (const auto &[key, client] : m_router.clients) {
        client->async_write(s);
    }
}