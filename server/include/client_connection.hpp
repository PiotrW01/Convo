#pragma once
#include <poll.h>
#include <vector>

class ClientConnection {
  public:
    pollfd            fd;
    std::vector<char> buffer;
    ClientConnection(int socket) : buffer(1024), fd() {
        fd.fd     = socket;
        fd.events = POLLIN;
    };
};