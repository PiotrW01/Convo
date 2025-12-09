#include "client.hpp"
#include "protocol.hpp"
#include <cerrno>

Client::Client() {
    m_socket                        = socket(AF_INET, SOCK_STREAM, 0);
    m_serverAddress.sin_family      = AF_INET;
    m_serverAddress.sin_port        = htons(7777);
    m_serverAddress.sin_addr.s_addr = INADDR_ANY;
}

void Client::run() {
    m_interface.on_enter_cb = [this](std::string &input) {
        if (input.length() > 0) {
            sendMessage(input);
            input = "";
        }
    };

    std::thread connection_thread([this] {
        int status = connect(m_socket, reinterpret_cast<sockaddr *>(&m_serverAddress),
                             sizeof(m_serverAddress));
        if (status < 0) {
            m_interface.printMessage(strerror(errno));
            m_interface.on_enter_cb = [this](std::string &msg) { msg = ""; };
            return;
        }

        auto msg = m_interface.printMessage("Enter your username:");

        m_session.status             = ClientSession::CONNECTED;
        std::vector<uint8_t> &buffer = m_session.buffer;
        while (true) {
            std::vector<uint8_t> temp(8192);
            int                  bytesReceived = recv(m_socket, temp.data(), temp.size(), 0);
            if (bytesReceived <= 0) {
                break;
            }
            buffer.insert(buffer.end(), temp.begin(), temp.begin() + bytesReceived);

            while (buffer.size() >= sizeof(Proto::PacketHeader)) {
                Proto::PacketHeader hdr;
                memcpy(&hdr, buffer.data(), sizeof(hdr));

                uint16_t payloadSize = ntohs(hdr.length);

                if (buffer.size() < sizeof(hdr) + payloadSize)
                    break;

                std::vector<uint8_t> payload(buffer.begin() + sizeof(hdr),
                                             buffer.begin() + sizeof(hdr) + payloadSize);

                switch (hdr.id) {
                case Proto::ID::MESSAGE: {
                    Proto::Message msg = Proto::Message::deserialize(payload);
                    if (msg.username.starts_with('!'))
                        m_interface.printMessage(msg.message, msg.username,
                                                 ftxui::color(ftxui::Color::RedLight));
                    else if (msg.username == m_session.username) {
                        m_interface.printMessage(msg.message, msg.username,
                                                 ftxui::color(ftxui::Color::Cyan));
                        m_interface.scrollDown();
                    } else
                        m_interface.printMessage(msg.message, msg.username);
                    break;
                }
                case Proto::LOGIN: {
                    Proto::LoginRequest res = Proto::LoginRequest::deserialize(payload);
                    m_interface.printMessage("logged in!");
                    m_session.status   = ClientSession::LOGGED_IN;
                    m_session.username = res.username;
                    msg->Detach();
                    break;
                }
                default:
                    break;
                }

                buffer.erase(buffer.begin(), buffer.begin() + sizeof(hdr) + payloadSize);
            }
        }
        close(m_socket);
    });
    connection_thread.detach();
    m_interface.init();
}

void Client::sendMessage(std::string &msg) {
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
