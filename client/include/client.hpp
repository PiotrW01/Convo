#pragma once
#include "client_interface.hpp"
#include <netinet/in.h>

class Client {
  private:
    ClientInterface interface;
    int             clientSocket;
    sockaddr_in     serverAddress;

  public:
  public:
    Client();
    void run();
    void sendMessage(std::string &msg);
};