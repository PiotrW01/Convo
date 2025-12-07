#pragma once

#include <atomic>
#include <csignal>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

class Server {
  private:
    // std::thread           m_workerThreads[4];
    // static Server       s_instance;
    std::atomic<bool>   running        = 1;
    int                 m_serverSocket = -1;
    int                 m_serverPort   = 7777;
    std::vector<pollfd> m_fds;

  private:
    void workerFunc();
    void broadcastMessage(const std::string &msg);
    void processMessage(const std::string &msg, int clientFD);
    void handleConnections(std::vector<pollfd> &m_fds);
    void disconnectClient(int &i);
    void connectClient(int clientSocket);

  public:
    int exitCode = 0;

  public:
    Server(std::optional<int> port);
    static Server createServer(std::optional<int> port);
    void          run();
    static void   stop(int signal);
};
