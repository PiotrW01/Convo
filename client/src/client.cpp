#include "client.hpp"
#include "protocol.hpp"
#include <cerrno>

Client::Client() {
    m_socket                         = socket(AF_INET, SOCK_STREAM, 0);
    m_server_address.sin_family      = AF_INET;
    m_server_address.sin_port        = htons(7777);
    m_server_address.sin_addr.s_addr = INADDR_ANY;
}

void Client::run() {
    m_interface.on_enter_cb = [this](std::string &input) {
        if (input.length() > 0) {
            send_message(input);
            input = "";
        }
    };

    std::thread connection_thread([this] {
        int status = connect(m_socket, reinterpret_cast<sockaddr *>(&m_server_address),
                             sizeof(m_server_address));
        if (status < 0) {
            m_interface.print_message(strerror(errno));
            m_interface.on_enter_cb = [this](std::string &msg) { msg = ""; };
            return;
        }

        auto msg = m_interface.print_message("Enter your username:");

        m_session.status             = ClientSession::CONNECTED;
        std::vector<uint8_t> &buffer = m_session.buffer;
        while (true) {
            std::vector<uint8_t> temp(8192);
            int                  bytes_received = recv(m_socket, temp.data(), temp.size(), 0);
            if (bytes_received <= 0) {
                break;
            }
            buffer.insert(buffer.end(), temp.begin(), temp.begin() + bytes_received);

            while (buffer.size() >= sizeof(Proto::PacketHeader)) {
                Proto::PacketHeader hdr;
                memcpy(&hdr, buffer.data(), sizeof(hdr));

                uint16_t payload_size = ntohs(hdr.length);

                if (buffer.size() < sizeof(hdr) + payload_size)
                    break;

                std::vector<uint8_t> payload(buffer.begin() + sizeof(hdr),
                                             buffer.begin() + sizeof(hdr) + payload_size);

                switch (hdr.id) {
                case Proto::ID::MESSAGE: {
                    Proto::Message msg = Proto::Message::deserialize(payload);
                    if (msg.username.starts_with('!'))
                        m_interface.print_message(msg.message, msg.username,
                                                  ftxui::color(ftxui::Color::RedLight));
                    else if (msg.username == m_session.username) {
                        m_interface.print_message(msg.message, msg.username,
                                                  ftxui::color(ftxui::Color::Cyan));
                        m_interface.scroll_down();
                    } else
                        m_interface.print_message(msg.message, msg.username);
                    break;
                }
                case Proto::LOGIN: {
                    Proto::LoginRequest res = Proto::LoginRequest::deserialize(payload);
                    m_interface.print_message("logged in!");
                    m_session.status   = ClientSession::LOGGED_IN;
                    m_session.username = res.username;
                    msg->Detach();
                    break;
                }
                default:
                    break;
                }

                buffer.erase(buffer.begin(), buffer.begin() + sizeof(hdr) + payload_size);
            }
        }
        close(m_socket);
    });
    connection_thread.detach();
    m_interface.init();
}

void Client::send_message(std::string &msg) {
    if (m_session.status == ClientSession::CONNECTED) {
        std::vector<uint8_t> payload = Proto::LoginRequest::serialize(msg);
        Proto::PacketHeader  hdr(Proto::ID::LOGIN, payload.size());
        send(m_socket, &hdr, sizeof(hdr), 0);
        send(m_socket, payload.data(), payload.size(), 0);
        return;
    } else if (m_session.status == ClientSession::LOGGED_IN) {
        // send(clientSocket, msg.data(), msg.length(), 0);
        std::vector<uint8_t> payload = Proto::Message::serialize("", msg);
        Proto::PacketHeader  hdr(Proto::ID::MESSAGE, payload.size());
        send(m_socket, &hdr, sizeof(hdr), 0);
        send(m_socket, payload.data(), payload.size(), 0);
    }
}
