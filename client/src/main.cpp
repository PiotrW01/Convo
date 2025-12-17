#include <atomic>
#include <iostream>

#include "client.hpp"
#include "client_config.hpp"
std::atomic<bool> running(true);

int main(int argc, const char **argv) {
    Client c(ClientConfig::create_cfg(argc, argv));
    c.run();
    return 0;
}