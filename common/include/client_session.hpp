#pragma once
#include <cstdint>
#include <poll.h>
#include <string>
#include <vector>

class ClientSession {
  public:
    enum Status : uint8_t { DISCONNECTED, CONNECTED, LOGGED_IN };
    Status      status = DISCONNECTED;
    std::string username;
};