#include "client.hpp"
#include <bits/stdc++.h>
#include <cerrno>
#include <unistd.h>

Client::Client(ClientConfig cfg) : m_config(cfg), m_interface(*this) {}

void Client::run_connection() {
    m_interface.print_message("Connecting...");
    setup_routes();
    m_router.use_ssl();
    m_router.connect_and_run(
        m_config.server_ip, std::to_string(m_config.port), [&](ClientSession::Status new_status) {
            if (new_status == ClientSession::CONNECTED) {
                session.status = new_status;

                m_interface.clear_messages();
                m_interface.print_message("Connected to server!");
                m_interface.print_message("Enter your username:");
            } else {
                m_interface.print_message(ClientSession::status_to_string(new_status));
            }
        });
}

void Client::setup_routes() {
    m_router.on_packet<Proto::Login>(
        [this](const std::shared_ptr<Proto::Connection> &conn, const Proto::Login &req) {
            if (req.username.length() == 0) {
                m_interface.print_message("Something went wrong during login.");
                return;
            }
            session.status   = ClientSession::LOGGED_IN;
            session.username = req.username;
            m_interface.clear_messages();
            m_interface.print_message("Logged in!");
        });

    m_router.on_packet<Proto::Message>([this](const std::shared_ptr<Proto::Connection> &conn,
                                              const Proto::Message                     &req) {
        std::string username = req.username;
        if (username.length() == 0)
            username = "NULL";
        if (username.at(0) == '!')
            m_interface.print_message(req.message, username, ftxui::color(ftxui::Color::RedLight));
        else if (username == session.username) {
            m_interface.print_message(req.message, username, ftxui::color(ftxui::Color::Cyan));
            m_interface.scroll_down();
        } else
            m_interface.print_message(req.message, username);
    });
}
void Client::run() {
    std::thread connection_thread([this] { run_connection(); });
    connection_thread.detach();
    m_interface.init();
}

void Client::send_message(std::string &msg) {
    if (msg.at(0) == '/') {
        if (msg.length() == 1)
            return;
        parse_commands(msg);
    }

    if (session.status == ClientSession::LOGGED_IN) {
        Proto::Message req;
        req.username = "";
        req.message  = msg;
        m_router.send_packet(req);
    }
}

void Client::login(const std::string &username, const std::string &password) {
    if (session.status == ClientSession::CONNECTED) {
        Proto::Login req;
        req.username = username;
        req.password = password;
        m_router.send_packet(req);
    }
}

void Client::register_user(const std::string &username, const std::string &password) {
    Proto::Register req;
    req.username = username;
    req.password = password;
    m_router.send_packet(req);
}

void Client::parse_commands(std::string &msg) {
    std::string              command;
    std::vector<std::string> args;

    std::regex                 del(" ");
    std::sregex_token_iterator it(msg.begin(), msg.end(), del, -1);
    std::sregex_token_iterator end;

    command = *it;
    command = command.substr(1, command.length() - 1);
    it++;
    while (it != end && args.size() < 6) {
        args.push_back(*it);
        ++it;
    }

    if (command == "login") {
        if (args.size() < 2)
            return;
        login(args.at(0), args.at(1));
    } else if (command == "register") {
        if (args.size() < 2)
            return;
        register_user(args.at(0), args.at(1));
    } else if (command == "clear") {
        m_interface.clear_messages();
    } else if (command == "msg") {
        m_interface.print_message(command + " command is not implemented.");
    } else if (command == "reconnect") {
        m_interface.print_message(command + " command is not implemented.");
    }
}