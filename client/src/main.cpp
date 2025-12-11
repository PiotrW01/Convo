#include <atomic>
#include <iostream>

#include "client.hpp"
std::atomic<bool> running(true);

int main() {
    Client c;
    c.run();
    return 0;
}