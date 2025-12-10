#pragma once

#include "client_session.hpp"
#include <atomic>
#include <optional>
#include <poll.h>
#include <unordered_map>

class Server {
  private:
    std::unordered_map<int, ClientSession> m_client_sessions;
    std::vector<pollfd>                    m_fds;
    std::atomic<bool>                      m_running = 1;
    pollfd                                 m_server_poll;
    int                                    m_server_socket = -1;
    int                                    m_server_port   = 7777;

  private:
    void broadcast_message(const Proto::Message &msg);
    void handle_connections(std::vector<pollfd> &m_fds);
    void try_process_data(int fd, Proto::Bytes &client_buffer);
    void connect_client(int client_socket);
    void disconnect_client(int &it);

  public:
    Server(std::optional<int> port);
    void run();
};
