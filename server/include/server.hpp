#pragma once

#include "client_session.hpp"
#include "router.hpp"
#include "server_config.hpp"
#include <atomic>
#include <mariadb/conncpp.hpp>
#include <optional>
#include <poll.h>
#include <unordered_map>

class Server {
  private:
    const ServerConfig               m_config;
    std::atomic<bool>                m_running = 1;
    Proto::ServerRouter              m_router;
    std::unique_ptr<sql::Connection> m_db;

  private:
    void broadcast_message(Proto::Message &msg);

  public:
    ServerConfig config() const { return m_config; };
    Server(ServerConfig cfg);
    void run();
};
