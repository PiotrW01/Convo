#include "server.hpp"
#include "protocol.hpp"
#include <format>
#include <iostream>

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
    while (m_running) {
        int events = poll(m_fds.data(), m_fds.size(), 1000);
        if (events < 0) {
            perror("poll");
            break;
        }
        handleConnections(m_fds);
    }
}

void Server::broadcastMessage(const Proto::Message &msg) {
    std::vector<uint8_t> payload = msg.serialize();
    Proto::PacketHeader  hdr(Proto::ID::MESSAGE, payload.size());

    std::cout << std::format("[{}] ", msg.username) << msg.message << std::endl;

    for (auto &client : m_fds) {
        if (client.fd == m_serverSocket)
            continue;
        sendPacket(client.fd, hdr, payload);
    }
}

void Server::sendPacket(const int &fd, const Proto::PacketHeader &hdr,
                        const std::vector<uint8_t> payload) {
    send(fd, &hdr, sizeof(hdr), 0);
    send(fd, payload.data(), payload.size(), 0);
}

void Server::handleConnections(std::vector<pollfd> &m_fds) {
    for (int it = 0; it < m_fds.size(); it++) {
        if (!(m_fds[it].revents & POLLIN))
            continue;

        if (m_fds[it].fd == m_serverSocket) {
            int fd = accept(m_serverSocket, nullptr, nullptr);
            if (fd >= 0) {
                connectClient(fd);
            }
            continue;
        }

        std::vector<uint8_t> temp(8192);
        int                  bytesReceived = recv(m_fds[it].fd, temp.data(), temp.size(), 0);
        if (bytesReceived <= 0) {
            disconnectClient(it);
            continue;
        }

        auto &clientBuffer = m_clientSessions[m_fds[it].fd].buffer;
        clientBuffer.insert(clientBuffer.end(), temp.begin(), temp.begin() + bytesReceived);

        tryProcessData(m_fds[it].fd, clientBuffer);
        //  std::string msg(buffer, bytesReceived);
        // processMessage(msg, socket);
    }
}

void Server::tryProcessData(int fd, std::vector<uint8_t> &buffer) {
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
            msg.username       = m_clientSessions[fd].username;
            broadcastMessage(msg);
            break;
        }
        case Proto::ID::LOGIN: {
            Proto::LoginRequest req = Proto::LoginRequest::deserialize(payload);

            auto &client = m_clientSessions[fd];

            if (req.username == "" || client.status == ClientSession::LOGGED_IN)
                break;
            client.username = req.username;
            client.status   = ClientSession::LOGGED_IN;

            Proto::Message msg;
            msg.username = "Server";
            msg.message  = std::format("{} joined the channel.", req.username);

            Proto::PacketHeader hdr(Proto::LOGIN, payload.size());
            sendPacket(fd, hdr, payload);
            broadcastMessage(msg);
            break;
        }
        default:
            break;
        }

        buffer.erase(buffer.begin(), buffer.begin() + sizeof(hdr) + payloadSize);
    }
}

void Server::disconnectClient(int &it) {
    std::string msg_disconnect(
        std::format("{} left the channel.", m_clientSessions[m_fds[it].fd].username));
    close(m_fds[it].fd);
    m_clientSessions.erase(m_fds[it].fd);
    m_fds.erase(m_fds.begin() + it);

    // we reduce it by 1 so the handleConnections loop doesn't skip over the next client in the
    // shortened m_fds array
    it--;

    Proto::Message msg;
    msg.username = "Server";
    msg.message  = msg_disconnect;
    broadcastMessage(msg);
}

void Server::connectClient(int fd) {
    std::string msg_connect(std::format("Client connected with id: {}", fd));
    pollfd      clientPoll{};
    clientPoll.fd     = fd;
    clientPoll.events = POLLIN;
    m_fds.push_back(clientPoll);
    ClientSession session;
    session.status       = ClientSession::CONNECTED;
    m_clientSessions[fd] = session;

    std::cout << msg_connect << std::endl;
}
