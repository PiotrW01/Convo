#include "server.hpp"

int main() {
    Server s(7777);
    s.run();
    return 0;
}