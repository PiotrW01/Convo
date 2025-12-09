#include "server.hpp"
#include "protocol.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>

std::mutex coutMutex;
Server::Server(std::optional<int> port) {
    // std::signal(SIGINT, stop);
    // std::signal(SIGHUP, stop);
    // std::signal(SIGTERM, stop);

    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == -1) {
        std::cout << "\nCould not create server socket.\n";
        exit(-1);
    }

    sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons(m_serverPort);
    serverAddress.sin_family      = AF_INET;

    int yes = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    bind(m_serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress),
         sizeof(serverAddress));
    int status = listen(m_serverSocket, 128);
    if (status == -1) {
        std::cout << "\nCould not start listening on server socket.\n";
        exit(-1);
    }

    m_serverPoll.fd     = m_serverSocket;
    m_serverPoll.events = POLLIN;
    m_fds.push_back(m_serverPoll);
    //  for (std::thread &t : m_workerThreads) {
    //      t = std::thread(std::bind(&Server::workerFunc, this));
    //  }
}

void Server::run() {
    std::cout << "Server is running on port " << m_serverPort << std::endl;
    while (running) {
        int events = poll(m_fds.data(), m_fds.size(), 1000);
        if (events < 0) {
            perror("poll");
            break;
        }
        handleConnections(m_fds);
    }
}

/*
static void Server::stop(int signal) {
    std::cout << "Closing the server..." << std::endl;
    running = 0;
    for (auto &s : m_fds) {
        if (s.fd != -1) {
            close(s.fd);
            s.fd = -1;
        }
    }
}
*/

void Server::workerFunc() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Thread " << std::this_thread::get_id() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void Server::broadcastMessage(const std::string &msg) {
    std::vector<uint8_t> payload = Proto::Message::serialize(msg);
    Proto::PacketHeader  hdr(Proto::ID::MESSAGE, payload.size());

    for (auto &client : m_fds) {
        if (client.fd == m_serverSocket)
            continue;
        send(client.fd, &hdr, sizeof(hdr), 0);
        send(client.fd, payload.data(), payload.size(), 0);
    }
}

void Server::processMessage(const std::string &msg, int clientFD) {
    std::cout << "Received from " << m_fds[clientFD].fd << ": " << msg << std::endl;
    broadcastMessage(msg);
}

void Server::handleConnections(std::vector<pollfd> &m_fds) {
    for (int socket = 0; socket < m_fds.size(); socket++) {
        if (!(m_fds[socket].revents & POLLIN))
            continue;

        if (m_fds[socket].fd == m_serverSocket) {
            int clientSocket = accept(m_serverSocket, nullptr, nullptr);
            if (clientSocket >= 0) {
                std::cout << "someone connected!\n";
                connectClient(clientSocket);
            }
            continue;
        }

        auto  &clientBuffer  = m_clientBuffers[socket];
        size_t currentLength = clientBuffer.size();
        clientBuffer.resize(currentLength + 8192);

        int bytesReceived = recv(m_fds[socket].fd, clientBuffer.data() + currentLength, 8192, 0);
        if (bytesReceived <= 0) {
            disconnectClient(socket);
            continue;
        }

        clientBuffer.resize(currentLength + bytesReceived);

        tryProcessData(socket, clientBuffer, bytesReceived);
        //  std::string msg(buffer, bytesReceived);
        // processMessage(msg, socket);
    }
}

void Server::tryProcessData(int socket, std::vector<uint8_t> &buffer, int bytesReceived) {
    while (buffer.size() >= sizeof(Proto::PacketHeader)) {
        Proto::PacketHeader hdr;
        memcpy(&hdr, buffer.data(), sizeof(hdr));

        uint16_t payloadSize = ntohs(hdr.length);

        if (buffer.size() < sizeof(hdr) + payloadSize)
            break;

        // uint8_t *payload = buffer.data() + sizeof(hdr);
        std::vector<uint8_t> payload(buffer.begin() + sizeof(hdr),
                                     buffer.begin() + sizeof(hdr) + payloadSize);

        // process packet
        // processPacket(hdr.id)
        switch (hdr.id) {
        case Proto::ID::MESSAGE: {
            Proto::Message msg = Proto::Message::deserialize(payload);
            std::cout << msg.message << " " << std::to_string(payloadSize) << std::endl;
            broadcastMessage(msg.message);
            break;
        }
        default:
            break;
        }

        buffer.erase(buffer.begin(), buffer.begin() + sizeof(hdr) + payloadSize);
    }
}

void Server::disconnectClient(int &socket) {
    std::string msg_disconnect("Client disconnected: " + std::to_string(m_fds[socket].fd));
    close(m_fds[socket].fd);
    m_fds.erase(m_fds.begin() + socket);
    m_clientBuffers.erase(socket);

    // we reduce i by 1 so the handleConnections loop doesn't skip over the next client in the
    // shortened m_fds array
    socket--;

    std::cout << msg_disconnect << std::endl;
    broadcastMessage(msg_disconnect);
}

void Server::connectClient(int socket) {
    std::string msg_connect("Client connected: " + std::to_string(socket));
    pollfd      clientPoll{};
    clientPoll.fd     = socket;
    clientPoll.events = POLLIN;
    m_fds.push_back(clientPoll);
    m_clientBuffers[socket] = std::vector<uint8_t>(1024);

    std::cout << msg_connect << std::endl;
    broadcastMessage(msg_connect);
}
