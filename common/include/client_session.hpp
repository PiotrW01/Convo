#pragma once
#include <cstdint>
#include <string>
#include <vector>

class ClientSession {
  public:
    enum Status : uint8_t { DISCONNECTED, CONNECTED, LOGGED_IN };

  public:
    ClientSession(size_t bufferSize = 0) : buffer(bufferSize){};
    Status               status = DISCONNECTED;
    std::string          username;
    std::vector<uint8_t> buffer;
};