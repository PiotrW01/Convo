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
        m_session.status = ClientSession::CONNECTED;
        auto msg         = m_interface.print_message("Enter your username:");

        Proto::Bytes &buffer = m_session.buffer;
        while (true) {
            int bytes = Proto::receive_data(m_socket, buffer);
            if (bytes <= 0)
                break;

            while (Proto::is_header_ready(buffer)) {
                Proto::PacketHeader hdr(buffer, Proto::Endianness::NETWORK_TO_HOST);
                Proto::PayloadSize  payload_size = hdr.length;
                if (!Proto::is_packet_ready(buffer, payload_size))
                    break;

                Proto::Payload payload = Proto::get_payload(buffer, payload_size);
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
                    m_session.status        = ClientSession::LOGGED_IN;
                    m_session.username      = res.username;
                    msg->Detach();
                    break;
                }
                default:
                    break;
                }

                Proto::remove_packet(buffer, payload_size);
            }
        }
        close(m_socket);
    });
    connection_thread.detach();
    m_interface.init();
}

void Client::send_message(std::string &msg) {
    Proto::Payload payload;

    if (m_session.status == ClientSession::CONNECTED) {
        m_interface.print_message("sending");
        Proto::Payload payload = Proto::LoginRequest::serialize(msg);
        Proto::send_packet(m_socket, payload, Proto::ID::LOGIN);
        return;
    } else if (m_session.status == ClientSession::LOGGED_IN) {
        // send(clientSocket, msg.data(), msg.length(), 0);
        Proto::Payload payload = Proto::Message::serialize("", msg);
        Proto::send_packet(m_socket, payload, Proto::ID::MESSAGE);
    }
}
