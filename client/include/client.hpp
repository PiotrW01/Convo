#pragma once
#include "client_interface.hpp"
#include "client_session.hpp"
#include "protocol.hpp"
#include <netinet/in.h>

class Client {
  private:
    ClientSession   m_session;
    ClientInterface m_interface;
    int             m_socket;
    sockaddr_in     m_server_address;

  public:
    Client();
    void run();
    void send_message(std::string &msg);
};