#include "server.hpp"

int main(int argc, const char **argv) {
    Server s(ServerConfig::create_cfg(argc, argv));
    s.run();
    return 0;
}