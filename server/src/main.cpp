#include "server.hpp"

int main() {
    Server s(std::nullopt);
    s.run();
    return 0;
}