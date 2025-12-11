#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <string>

class Client;

class ClientInterface {
  private:
    ftxui::ScreenInteractive m_screen;
    ftxui::Component         m_messages_container;
    ftxui::Component         m_messages_renderer;
    ftxui::Component         m_input_component;
    ftxui::Component         m_main_container;
    float                    m_scroll_offset = 1.0f;
    Client                  &m_client;

  private:
    ftxui::Component create_main_container();
    void             style_input();
    void             style_messages();
    void             on_enter_cb(std::string &user_input);

  public:
    std::string user_input;

  public:
    ClientInterface(Client &client);
    void init();
    void refresh();
    void scroll_down();
    void clear_messages();
    ftxui::Component
    print_message(const std::string &msg, const std::string &user = "System",
                  ftxui::Decorator name_color = ftxui::color(ftxui::Color::GrayLight));
};