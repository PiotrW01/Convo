#include "server.hpp"

void handleConnections(std::vector<pollfd> &g_fds);
void stopServer(int signal);
void startServer();

int main() {

    Server s(std::nullopt);
    s.run();

    return s.exitCode;
}

/*void stopServer(int signal) {
    // don't use exit - clean it up
    running = 0;
    std::cout << "\nShutting down...\n";
    if (g_serverSocket != -1) {
        close(g_serverSocket);
    }
    exit(0);
}*/