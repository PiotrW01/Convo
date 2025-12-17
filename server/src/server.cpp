#include "server.hpp"

#include "bcrypt/BCrypt.hpp"
#include "fmt/format.h"
#include "s_logger.hpp"
#include <bits/stdc++.h>
#include <iostream>
#include <openssl/evp.h>
#include <unistd.h>

Server::Server(ServerConfig cfg) : m_config(cfg), m_router(cfg.port) {
    // std::signal(SIGINT, stop);
    // std::signal(SIGHUP, stop);
    // std::signal(SIGTERM, stop);
}

void Server::run() {
    connect_to_database();
    setup_routes();

    Logger::server_message(fmt::format("Server is running on port {}", 7777));
    m_router.use_ssl(m_config.cert_pem_path, m_config.cert_key_path);
    m_router.run();
}

void Server::setup_routes() {
    m_router.on_packet<Proto::Login>([this](std::shared_ptr<Proto::Connection> conn,
                                            Proto::Login                      &packet) {
        auto stmt(m_db->prepareStatement("select password_hash from accounts where username = ?"));
        stmt->setString(1, packet.username);
        auto res(stmt->executeQuery());
        if (res->next()) {
            bool is_pwd_valid =
                BCrypt::validatePassword(packet.password, res->getString("password_hash").c_str());
            if (is_pwd_valid) {
                conn->client_session.username = packet.username;
                conn->client_session.status   = ClientSession::LOGGED_IN;
                Proto::Login req;
                req.username = packet.username;
                m_router.send_packet(conn, req);
                Proto::Message msg;
                msg.username = "Server";
                msg.message  = req.username + " joined the channel";
                broadcast_message(msg);
                return;
            }
        }
        send_message(conn, "Invalid password or username.");
    });

    m_router.on_packet<Proto::Register>(
        [this](std::shared_ptr<Proto::Connection> conn, Proto::Register &packet) {
            for_each(packet.username.begin(), packet.username.end(),
                     [](char &c) { c = std::tolower(c); });
            if (packet.username == "server" || packet.username == "system") {
                send_message(conn, "Invalid username.");
                return;
            }
            if (packet.username.length() < 3 || packet.username.length() > 24) {
                send_message(conn, "Username has to be in range 3-24 characters.");
                return;
            }
            if (packet.password.length() < 4 || packet.password.length() > 32) {
                send_message(conn, "Password has to be in range 4-32 characters.");
                return;
            }

            auto stmt(m_db->prepareStatement("select count(*) from accounts where username = ?"));
            stmt->setString(1, packet.username);
            auto res(stmt->executeQuery());
            if (res->next()) {
                int count = res->getUInt(1);
                if (count != 0) {
                    send_message(conn, "User already exists.");
                    return;
                }

                if (count == 0) {
                    std::string pwd_hash = BCrypt::generateHash(packet.password, 14);

                    auto stmt2(m_db->prepareStatement(
                        "insert into accounts (username, password_hash) values (?, ?)"));
                    stmt2->setString(1, packet.username);
                    stmt2->setString(2, pwd_hash);
                    auto r = stmt2->executeUpdate();
                    if (r > 0) {
                        Proto::Message msg;
                        msg.username = "Server";
                        msg.message  = "Account created!";
                        m_router.send_packet(conn, msg);
                        return;
                    } else {
                        send_message(conn, "Something went wrong during account creation.");
                        return;
                    }
                }
            }

            // while (res->next()) {
            //     std::cout << fmt::format("{} {} {} {}", res->getInt(1),
            //     res->getString(2).c_str(),
            //                              res->getString(3).c_str(), res->getString(4).c_str())
            //               << std::endl;
            // }
        });
    m_router.on_packet<Proto::Message>(
        [this](std::shared_ptr<Proto::Connection> conn, Proto::Message &packet) {
            if (conn->client_session.status != ClientSession::LOGGED_IN)
                return;

            packet.username = conn->client_session.username;
            broadcast_message(packet);
        });
}

void Server::broadcast_message(Proto::Message &msg) {
    std::shared_ptr<Proto::Bytes> s = std::make_shared<Proto::Bytes>(msg.serialize());
    for (const auto &[key, client] : m_router.clients) {
        if (client->client_session.status == ClientSession::LOGGED_IN)
            client->async_write(s);
    }
}

void Server::connect_to_database() {
    sql::Driver *driver = sql::mariadb::get_driver_instance();
    m_db                = std::unique_ptr<sql::Connection>(
        driver->connect(m_config.db_address, m_config.db_user, m_config.db_password));
    m_db->setSchema(m_config.db_schema);
}

void Server::send_error(std::shared_ptr<Proto::Connection> conn, const std::string &description,
                        Proto::PACKET_ERROR code) {
    Proto::Error err;
    err.error_code        = code;
    err.error_description = description;
    m_router.send_packet(conn, err);
}

void Server::send_message(std::shared_ptr<Proto::Connection> conn, const std::string &message) {
    Proto::Message msg;
    msg.username = "Server";
    msg.message  = message;
    m_router.send_packet(conn, msg);
}