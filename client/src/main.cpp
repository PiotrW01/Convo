#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(7777);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress));

    const std::string message = "Hello from client";
    send(clientSocket, message.data(), message.length(), 0);
    char buffer[1024];
    int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << buffer << std::endl;
    close(clientSocket);

    return 0;
}