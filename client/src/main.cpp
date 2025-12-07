#include <atomic>
#include <iostream>

#include "client.hpp"
std::atomic<bool> running(true);

int main() {
    Client c;
    c.run();
    std::cout << "Client exit" << std::endl;
    return 0;
}