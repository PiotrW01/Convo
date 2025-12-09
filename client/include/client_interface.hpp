#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <string>

class ClientInterface {
  private:
    ftxui::ScreenInteractive m_screen;
    ftxui::Component         m_messages_container;
    ftxui::Component         m_messages_renderer;
    ftxui::Component         m_input_component;
    ftxui::Component         m_main_container;
    float                    m_scroll_offset = 1.0f;

  private:
    ftxui::Component createMainContainer();
    void             styleInput();
    void             styleMessages();

  public:
    std::function<void(std::string &)> on_enter_cb;
    std::string                        user_input;

  public:
    ClientInterface();
    void init();
    void refresh();
    void scrollDown();
    ftxui::Component
    printMessage(const std::string &msg, const std::string &user = "System",
                 ftxui::Decorator namecolor = ftxui::color(ftxui::Color::GrayLight));
};