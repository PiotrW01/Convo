#include <iostream>

#include "ftxui/component/component.hpp"
#include "protocol.hpp"
#include <cerrno>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "client.hpp"

Client::Client() {
    clientSocket                  = socket(AF_INET, SOCK_STREAM, 0);
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_port        = htons(7777);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
}

void Client::run() {
    interface.on_enter_cb = [this](std::string &input) {
        if (input.length() > 0) {
            sendMessage(input);
            input = "";
        }
        // interface.printMessage(msg);
    };

    std::thread connection_thread([this] {
        int status = connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress),
                             sizeof(serverAddress));
        if (status < 0) {
            interface.printMessage("Disconnected");
            interface.printMessage(strerror(errno));
            interface.on_enter_cb = [this](std::string &msg) { msg = ""; };
            return;
        }

        std::vector<uint8_t> buffer;

        while (true) {
            size_t currentLength = buffer.size();
            buffer.resize(currentLength + 8192);

            int bytesReceived = recv(clientSocket, buffer.data() + currentLength, 8192, 0);
            if (bytesReceived <= 0) {
                break;
            }
            buffer.resize(currentLength + bytesReceived);

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
                    interface.printMessage(msg.message);
                }
                default:
                    break;
                }

                buffer.erase(buffer.begin(), buffer.begin() + sizeof(hdr) + payloadSize);
            }
        }
        close(clientSocket);
    });
    connection_thread.detach();
    interface.init();
}

void Client::sendMessage(std::string &msg) {
    // send(clientSocket, msg.data(), msg.length(), 0);
    std::vector<uint8_t> payload = Proto::Message::serialize(msg);
    Proto::PacketHeader  hdr(Proto::ID::MESSAGE, payload.size());
    send(clientSocket, &hdr, sizeof(hdr), 0);
    send(clientSocket, payload.data(), payload.size(), 0);
}
