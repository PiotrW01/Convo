#include <iostream>

#include "ftxui/component/component.hpp"
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
            interface.on_enter_cb = [this](std::string &msg) { msg = ""; };
            return;
        }

        while (true) {
            char buffer[1024];
            int  bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytes <= 0)
                break;
            interface.printMessage(std::string(buffer, bytes));
        }
        close(clientSocket);
    });
    connection_thread.detach();
    interface.init();
}

void Client::sendMessage(std::string &msg) {
    send(clientSocket, msg.data(), msg.length(), 0);
}
