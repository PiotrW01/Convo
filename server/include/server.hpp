#pragma once

#include "client_session.hpp"
#include "router.hpp"
#include <atomic>
#include <optional>
#include <poll.h>
#include <unordered_map>

class Server {
  private:
    std::atomic<bool>   m_running = 1;
    Proto::ServerRouter m_router;

  private:
    void broadcast_message(Proto::Message &msg);

  public:
    Server(int port);
    void run();
};
