#pragma once
#include "client_interface.hpp"
#include "client_session.hpp"
#include "router.hpp"
#include <functional>
#include <netinet/in.h>

class Client {
  private:
    ClientInterface     m_interface;
    Proto::ClientRouter m_router;

  private:
    void run_connection();
    void on_login_request_cb(const std::shared_ptr<Proto::Connection> &conn,
                             const Proto::Login                       &req);
    void on_message_cb(const std::shared_ptr<Proto::Connection> &conn, const Proto::Message &req);

  public:
    ClientSession session;

  public:
    Client();
    void run();
    void send_message(std::string &msg);
    void login(std::string &msg);
};