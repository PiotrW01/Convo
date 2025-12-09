#include "client_interface.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

ClientInterface::ClientInterface() : m_screen(ScreenInteractive::Fullscreen()) {
    messages_container = Container::Vertical({});
    input_component    = ftxui::Input(&user_input, "Enter message",
                                      InputOption{.multiline = false,
                                                  .on_enter  = [this] { on_enter_cb(user_input); }}) |
                      borderRounded | size(HEIGHT, EQUAL, 3);
    messages_renderer = Renderer(messages_container, [&] {
        Elements elems;
        for (int i = messages_container->ChildCount() - 1; i >= 0; i--) {
            elems.push_back(messages_container->ChildAt(i)->Render());
        }
        auto f = flexbox(elems,
                         FlexboxConfig{
                             .direction       = FlexboxConfig::Direction::ColumnInversed,
                             .wrap            = FlexboxConfig::Wrap::NoWrap,
                             .justify_content = FlexboxConfig::JustifyContent::FlexStart,
                             .align_items     = FlexboxConfig::AlignItems::FlexStart,
                         }) |
                 flex;
        return yframe(f);
        // return vbox(elems) | flex;
    });
}

void ClientInterface::printMessage(const std::string &msg) {
    std::string str = msg;
    auto        t   = Renderer(
        [str] { return vbox({paragraph("[User]") | color(Color::Red), paragraph(" " + str)}); });
    messages_container->Add(t);
    m_screen.PostEvent(Event::Custom);
}

void ClientInterface::init() {
    auto separator      = Renderer([] { return separatorHeavy(); });
    auto main_container = Container::Vertical({
        messages_renderer,
        separator,
        input_component,
    });
    input_component->TakeFocus();
    m_screen.Loop(main_container);
}
