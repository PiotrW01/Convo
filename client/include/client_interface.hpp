#pragma once
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <string>

class ClientInterface {
  private:
    ftxui::ScreenInteractive m_screen;

  public:
    std::string      user_input;
    ftxui::Component messages_container;
    ftxui::Component messages_renderer;
    ftxui::Component input_component;
    ftxui::Component main_container;

  public:
    ClientInterface();
    std::function<void(std::string &)> on_enter_cb;

    void printMessage(const std::string &msg);
    void init();
};