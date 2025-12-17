#pragma once
#include <cstdint>
#include <poll.h>
#include <string>
#include <vector>

class ClientSession {
  public:
    enum Status : uint8_t {
        DISCONNECTED,
        CONNECTED,
        LOGGED_IN,
        HANDSHAKE_FAILED,
        CONNECTION_FAILED
    };
    Status      status = DISCONNECTED;
    std::string username;

    static const char *status_to_string(Status s) {
        switch (s) {
        case Status::CONNECTED:
            return "Connected";
        case Status::DISCONNECTED:
            return "Disconnected";
        case Status::LOGGED_IN:
            return "Logged in";
        case Status::CONNECTION_FAILED:
            return "Connection failed";
        case Status::HANDSHAKE_FAILED:
            return "Handshake failed";
        }
        return "Unknown";
    }
};