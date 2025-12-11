#include "client_interface.hpp"
#include "client.hpp"
#include "fmt/format.h"

using namespace ftxui;

void ClientInterface::style_input() {
    InputOption opt;
    opt.multiline = false;
    opt.on_enter  = [this] { on_enter_cb(user_input); };
    opt.transform = [](InputState state) {
        Element element = state.element;
        if (state.focused) {
            Color text_color;
            if (state.is_placeholder)
                text_color = Color(122, 122, 122);
            else
                text_color = Color(230, 230, 230);

            return element | bgcolor(Color(0xff, 0, 0, 20)) | color(text_color) | bold;
        } else {
            return element | bgcolor(Color::Black) | color(Color::GrayLight);
        }
    };

    m_input_component = ftxui::Input(&user_input, "Enter message", opt) | borderRounded |
                        color(Color(0xff, 0x8f, 0xa6)) | size(HEIGHT, EQUAL, 3);
}

void ClientInterface::on_enter_cb(std::string &user_input) {
    if (user_input.length() > 0) {

        if (m_client.session.status == ClientSession::CONNECTED) {
            m_client.login(user_input);
        } else {
            m_client.send_message(user_input);
        }

        user_input = "";
    }
}

void ClientInterface::style_messages() {
    m_messages_container = Container::Vertical({});
    m_messages_renderer  = Renderer(m_messages_container, [&] {
        Elements elems;

        // * We add a temporary(?) filler element because frame somehow
        // * messes up message text display when the height of the text is too big
        // ! setting filler width equal 1 causes message border to not stretch properly
        elems.push_back(filler() | size(HEIGHT, EQUAL, 1));

        for (int i = m_messages_container->ChildCount() - 1; i >= 0; i--) {
            elems.push_back(m_messages_container->ChildAt(i)->Render());
        }

        auto f = flexbox(elems,
                          FlexboxConfig{
                              .direction       = FlexboxConfig::Direction::ColumnInversed,
                              .wrap            = FlexboxConfig::Wrap::NoWrap,
                              .justify_content = FlexboxConfig::JustifyContent::FlexStart,
                              .align_items     = FlexboxConfig::AlignItems::FlexStart,
                         }) |
                 flex;
        return f | vscroll_indicator | focusPositionRelative(0, m_scroll_offset) | yframe |
               borderStyled(Color(0xff, 0x8f, 0xa6));
    });
}

Component ClientInterface::create_main_container() {
    std::function<bool(Event)> scroll_callback = [this](Event event) {
        if (event.is_mouse()) {
            // printMessage("wheel uppies!");
            if (event.mouse().button == Mouse::Button::WheelUp) {
                m_scroll_offset -= 0.1f;
            } else if (event.mouse().button == Mouse::Button::WheelDown) {
                m_scroll_offset += 0.1f;
            }
            m_scroll_offset = std::clamp(m_scroll_offset, 0.0f, 1.0f);
            return true;
        }
        return false;
    };

    auto separator = Renderer([] { return separatorHeavy(); });
    return Container::Vertical({
               m_messages_renderer,
               separator,
               m_input_component,
           }) |
           bgcolor(Color(0x0f, 0x00, 0x20)) | CatchEvent(scroll_callback);
}

ClientInterface::ClientInterface(Client &client)
    : m_screen(ScreenInteractive::Fullscreen()), m_client(client) {
    style_input();
    style_messages();
}

void ClientInterface::refresh() {
    m_screen.PostEvent(Event::Custom);
}

void ClientInterface::scroll_down() {
    m_scroll_offset = 1.0f;
}

void ClientInterface::clear_messages() {
    m_messages_container->DetachAllChildren();
}

Component ClientInterface::print_message(const std::string &msg, const std::string &user,
                                         Decorator name_color) {
    std::string str     = msg;
    auto        message = Renderer([str, user, name_color] {
        return vbox({paragraph(fmt::format("[{}]", user)) | name_color, paragraph(" " + str)}) |
               borderStyled(Color(0xff, 0x8f, 0xa6)) | xflex;
    });
    m_messages_container->Add(message);
    m_screen.PostEvent(Event::Custom);
    return message;
}

void ClientInterface::init() {
    auto cont = create_main_container();
    m_input_component->TakeFocus();
    m_screen.Loop(cont);
}
