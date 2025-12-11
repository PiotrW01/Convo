#include "client.hpp"
#include "protocol.hpp"
#include <cerrno>

Client::Client() : m_interface(*this) {

    m_socket                         = socket(AF_INET, SOCK_STREAM, 0);
    m_server_address.sin_family      = AF_INET;
    m_server_address.sin_port        = htons(7777);
    m_server_address.sin_addr.s_addr = INADDR_ANY;
}

void Client::run_connection() {
    int status = connect(m_socket, reinterpret_cast<sockaddr *>(&m_server_address),
                         sizeof(m_server_address));
    if (status < 0) {
        m_interface.print_message(strerror(errno));
        return;
    }
    m_session.status = ClientSession::CONNECTED;
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
    m_session.status   = ClientSession::LOGGED_IN;
    m_session.username = req.username;
    m_interface.clear_messages();
};
void Client::on_message_cb(const int fd, const Proto::Message &req) {
    if (req.username.starts_with('!'))
        m_interface.print_message(req.message, req.username, ftxui::color(ftxui::Color::RedLight));
    else if (req.username == m_session.username) {
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
    if (m_session.status == ClientSession::LOGGED_IN) {
        // send(clientSocket, msg.data(), msg.length(), 0);
        Proto::Payload payload = Proto::Message::serialize("", msg);
        Proto::send_packet(m_socket, payload, Proto::ID::MESSAGE);
    }
}

void Client::login(std::string &msg) {
    if (m_session.status == ClientSession::CONNECTED) {
        Proto::Payload payload = Proto::LoginRequest::serialize(msg);
        Proto::send_packet(m_socket, payload, Proto::ID::LOGIN);
    }
}
