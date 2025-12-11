#include "client.hpp"
#include "protocol.hpp"
#include <cerrno>
#include <unistd.h>

Client::Client() : m_interface(*this) {

    m_socket                         = socket(AF_INET, SOCK_STREAM, 0);
    m_server_address.sin_family      = AF_INET;
    m_server_address.sin_port        = htons(7777);
    m_server_address.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "192.168.31.166", &m_server_address.sin_addr);
}

void Client::run_connection() {
    int status = connect(m_socket, reinterpret_cast<sockaddr *>(&m_server_address),
                         sizeof(m_server_address));
    if (status < 0) {
        m_interface.print_message(strerror(errno));
        return;
    }
    session.status = ClientSession::CONNECTED;
    m_interface.print_message("Enter your username:");

    Proto::ClientRouter router;
    router.on_packet<Proto::LoginRequest>(
        [this](const int fd, const Proto::LoginRequest &req) { on_login_request_cb(fd, req); });
    router.on_packet<Proto::Message>(
        [this](const int fd, const Proto::Message &req) { on_message_cb(fd, req); });

    router.run(m_socket);
    close(m_socket);
}
void Client::on_login_request_cb(const int fd, const Proto::LoginRequest &req) {
    session.status   = ClientSession::LOGGED_IN;
    session.username = req.username;
    m_interface.clear_messages();
};
void Client::on_message_cb(const int fd, const Proto::Message &req) {
    if (req.username.at(0) == '!')
        m_interface.print_message(req.message, req.username, ftxui::color(ftxui::Color::RedLight));
    else if (req.username == session.username) {
        m_interface.print_message(req.message, req.username, ftxui::color(ftxui::Color::Cyan));
        m_interface.scroll_down();
    } else
        m_interface.print_message(req.message, req.username);
};

void Client::run() {
    std::thread connection_thread([this] { run_connection(); });
    connection_thread.detach();
    m_interface.init();
}

void Client::send_message(std::string &msg) {
    if (session.status == ClientSession::LOGGED_IN) {
        Proto::Payload payload = Proto::Message::serialize("", msg);
        Proto::send_packet(m_socket, payload, Proto::ID::MESSAGE);
    }
}

void Client::login(std::string &msg) {
    if (session.status == ClientSession::CONNECTED) {
        Proto::Payload payload = Proto::LoginRequest::serialize(msg);
        Proto::send_packet(m_socket, payload, Proto::ID::LOGIN);
    }
}
