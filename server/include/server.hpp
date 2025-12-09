#pragma once

#include "client_session.hpp"
#include "protocol.hpp"
#include <atomic>
#include <optional>
#include <poll.h>
#include <unordered_map>

class Server {
  private:
    std::unordered_map<int, ClientSession> m_clientSessions;
    std::vector<pollfd>                    m_fds;
    std::atomic<bool>                      m_running = 1;
    pollfd                                 m_serverPoll;
    int                                    m_serverSocket = -1;
    int                                    m_serverPort   = 7777;

  private:
    void broadcastMessage(const Proto::Message &msg);
    void handleConnections(std::vector<pollfd> &m_fds);
    void tryProcessData(int fd, std::vector<uint8_t> &clientBuffer);
    void connectClient(int clientSocket);
    void disconnectClient(int &it);
    void sendPacket(const int &fd, const Proto::PacketHeader &hdr,
                    const std::vector<uint8_t> payload);

  public:
    Server(std::optional<int> port);
    void run();
};
