#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <poll.h>
#include <vector>

void stopServer(int signal);
void startServer();
volatile sig_atomic_t running = 1;

int g_serverSocket = -1;
int g_serverPort = 7777;

int main()
{
    std::signal(SIGINT, stopServer);
    std::signal(SIGHUP, stopServer);
    std::signal(SIGTERM, stopServer);
    startServer();

    std::vector<pollfd> fds;
    pollfd serverPoll{};
    serverPoll.fd = g_serverSocket;
    serverPoll.events = POLLIN;
    fds.push_back(serverPoll);

    char buffer[1024] = {0};
    while (running)
    {
        int events = poll(fds.data(), fds.size(), 1000);
        if (events < 0)
        {
            perror("poll");
            break;
        }

        for (int i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == g_serverSocket)
                {
                    int clientSocket = accept(g_serverSocket, nullptr, nullptr);
                    if (clientSocket >= 0)
                    {
                        std::cout << "Client connected: " << clientSocket << std::endl;
                        pollfd clientPoll{};
                        clientPoll.fd = clientSocket;
                        clientPoll.events = POLLIN;
                        fds.push_back(clientPoll);
                    }
                }
                else
                {
                    char buffer[1024];
                    // removing 1 byte of space from the received buffer size
                    // so we have room for the null terminator
                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes <= 0)
                    {
                        std::cout << "Client disconnected: " << fds[i].fd << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        buffer[bytes] = '\0';
                        std::cout << "Received from " << fds[i].fd << ": " << buffer << std::endl;
                        send(fds[i].fd, buffer, bytes, 0);
                    }
                }
            }
        }
    }

    return 0;
}

void startServer()
{
    g_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_serverSocket == -1)
    {
        std::cout << "\nCould not create server socket.\n";
        exit(-1);
    }

    sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(g_serverPort);
    serverAddress.sin_family = AF_INET;

    bind(g_serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress));
    int status = listen(g_serverSocket, 128);
    if (status == -1)
    {
        std::cout << "\nCould not start listening on server socket.\n";
        exit(-1);
    }
    std::cout << "Server is open on port " << g_serverPort << std::endl;
}

void stopServer(int signal)
{
    if (signal == SIGINT)
    {
        running = 0;
        std::cout << "\nShutting down...\n";
        if (g_serverSocket != -1)
        {
            close(g_serverSocket);
        }
        exit(0);
    }
}