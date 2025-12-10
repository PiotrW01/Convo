#pragma once
#include "protocol.hpp"
#include <cstdint>
#include <poll.h>
#include <string>
#include <vector>

class ClientSession {
  public:
    enum Status : uint8_t { DISCONNECTED, CONNECTED, LOGGED_IN };
    Status       status = DISCONNECTED;
    std::string  username;
    Proto::Bytes buffer;
    ClientSession(size_t buffer_size = 0) : buffer(buffer_size){};
};