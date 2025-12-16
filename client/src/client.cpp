#include "client.hpp"
#include <cerrno>
#include <unistd.h>

Client::Client() : m_interface(*this) {}

void Client::run_connection() {
    m_router.on_packet<Proto::Login>(
        [this](const std::shared_ptr<Proto::Connection> &conn, const Proto::Login &req) {
            on_login_request_cb(conn, req);
        });
    m_router.on_packet<Proto::Message>(
        [this](const std::shared_ptr<Proto::Connection> &conn, const Proto::Message &req) {
            on_message_cb(conn, req);
        });
    session.status = ClientSession::CONNECTED;
    m_interface.print_message("Enter your username:");
    m_router.use_ssl();
    m_router.connect("127.0.0.1", "7777");

    // m_router.run(m_socket);
    // close(m_socket);
}
void Client::on_login_request_cb(const std::shared_ptr<Proto::Connection> &conn,
                                 const Proto::Login                       &req) {
    session.status   = ClientSession::LOGGED_IN;
    session.username = req.username;
    m_interface.clear_messages();
    m_interface.print_message("Logged in!!!");
};
void Client::on_message_cb(const std::shared_ptr<Proto::Connection> &conn,
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
};

void Client::run() {
    std::thread connection_thread([this] { run_connection(); });
    connection_thread.detach();
    m_interface.init();
}

void Client::send_message(std::string &msg) {
    if (session.status == ClientSession::LOGGED_IN) {
        Proto::Message req;
        req.username                    = "";
        req.message                     = msg;
        std::shared_ptr<Proto::Bytes> s = std::make_shared<Proto::Bytes>(req.serialize());
        m_router.server->async_write(s);
    }
}

void Client::login(std::string &username) {
    if (session.status == ClientSession::CONNECTED) {
        Proto::Login req;
        req.username                    = username;
        std::shared_ptr<Proto::Bytes> s = std::make_shared<Proto::Bytes>(req.serialize());
        m_router.server->async_write(s);

        // Proto::Payload payload = Proto::LoginRequest::serialize(username);
        // Proto::send_packet(m_socket, payload, Proto::ID::LOGIN);
    }
}
