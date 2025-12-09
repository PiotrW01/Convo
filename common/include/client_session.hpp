#pragma once
#include <cstdint>
#include <poll.h>
#include <string>
#include <vector>

class ClientSession {
  public:
    enum Status : uint8_t { DISCONNECTED, CONNECTED, LOGGED_IN };
    Status               status = DISCONNECTED;
    std::string          username;
    std::vector<uint8_t> buffer;
    ClientSession(size_t bufferSize = 0) : buffer(bufferSize){};
};