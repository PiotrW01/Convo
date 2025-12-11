#pragma once
#include "client_interface.hpp"
#include "client_session.hpp"
#include "protocol.hpp"
#include <functional>
#include <netinet/in.h>

class Client {
  private:
    ClientSession   m_session;
    ClientInterface m_interface;
    int             m_socket;
    sockaddr_in     m_server_address;

  private:
    void run_connection();
    void on_login_request_cb(const int fd, const Proto::LoginRequest &req);
    void on_message_cb(const int fd, const Proto::Message &req);

  public:
    Client();
    void run();
    void send_message(std::string &msg);
    void login(std::string &msg);
};