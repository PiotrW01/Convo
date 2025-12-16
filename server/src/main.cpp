#include "server.hpp"

#ifndef SERVER_NAME
#define SERVER_NAME "ConvoServer"
#endif

#ifndef SERVER_VERSION
#define SERVER_VERSION "0.0.1"
#endif

int main(int argc, const char **argv) {
    Server s(ServerConfig::create_cfg(argc, argv));
    s.run();
    return 0;
}