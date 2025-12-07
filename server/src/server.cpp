#include "server.hpp"
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

    bind(m_serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress),
         sizeof(serverAddress));
    int status = listen(m_serverSocket, 128);
    if (status == -1) {
        std::cout << "\nCould not start listening on server socket.\n";
        exit(-1);
    }

    pollfd serverPoll{};
    serverPoll.fd     = m_serverSocket;
    serverPoll.events = POLLIN;
    m_fds.push_back(serverPoll);

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
    for (auto &client : m_fds) {
        if (client.fd == m_serverSocket)
            continue;
        send(client.fd, msg.data(), msg.size(), 0);
    }
}

void Server::processMessage(const std::string &msg, int clientFD) {
    std::cout << "Received from " << m_fds[clientFD].fd << ": " << msg << std::endl;
    broadcastMessage(msg);
}

void Server::handleConnections(std::vector<pollfd> &m_fds) {
    for (int i = 0; i < m_fds.size(); i++) {
        if (!(m_fds[i].revents & POLLIN))
            continue;

        if (m_fds[i].fd == m_serverSocket) {
            int clientSocket = accept(m_serverSocket, nullptr, nullptr);
            if (clientSocket >= 0) {
                connectClient(clientSocket);
            }
        } else {
            char buffer[1024];
            int  bytes = recv(m_fds[i].fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                disconnectClient(i);
            } else {
                // will probably be offloaded to a worker thread?
                std::string msg(buffer, bytes);
                processMessage(msg, i);
            }
        }
    }
}

void Server::disconnectClient(int &i) {
    std::string msg_disconnect("Client disconnected: " + std::to_string(m_fds[i].fd));
    close(m_fds[i].fd);
    m_fds.erase(m_fds.begin() + i);

    // we reduce i by 1 so the handleConnections loop doesn't skip over the next client in the
    // shortened m_fds array
    i--;

    std::cout << msg_disconnect << std::endl;
    broadcastMessage(msg_disconnect);
}

void Server::connectClient(int clientSocket) {
    std::string msg_connect("Client connected: " + std::to_string(clientSocket));
    pollfd      clientPoll{};
    clientPoll.fd     = clientSocket;
    clientPoll.events = POLLIN;
    m_fds.push_back(clientPoll);

    std::cout << msg_connect << std::endl;
    broadcastMessage(msg_connect);
}
