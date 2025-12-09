#pragma once

#include "client_session.hpp"
#include "protocol.hpp"
#include <atomic>
#include <csignal>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

class Server {
  private:
    // std::thread           m_workerThreads[4];
    // static Server       s_instance;
    std::atomic<bool>                      running        = 1;
    int                                    m_serverSocket = -1;
    int                                    m_serverPort   = 7777;
    std::vector<pollfd>                    m_fds;
    std::unordered_map<int, ClientSession> m_clientSessions;
    pollfd                                 m_serverPoll;
    // std::vector<ClientConnection> clients;

  private:
    void workerFunc();
    void broadcastMessage(const Proto::Message &msg);
    void handleConnections(std::vector<pollfd> &m_fds);
    void disconnectClient(int &i);
    void connectClient(int clientSocket);
    void tryProcessData(int fd, std::vector<uint8_t> &clientBuffer);
    void sendPacket(const int &fd, const Proto::PacketHeader &hdr,
                    const std::vector<uint8_t> payload);

  public:
    int exitCode = 0;

  public:
    Server(std::optional<int> port);
    static Server createServer(std::optional<int> port);
    void          run();
    static void   stop(int signal);
};
