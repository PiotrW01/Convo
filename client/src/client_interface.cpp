#include "client_interface.hpp"

using namespace ftxui;

void ClientInterface::styleInput() {
    InputOption opt;
    opt.multiline = false;
    opt.on_enter  = [this] { on_enter_cb(user_input); };
    opt.transform = [](InputState state) {
        Element element = state.element;
        if (state.focused) {
            Color textColor;
            if (state.is_placeholder)
                textColor = Color(122, 122, 122);
            else
                textColor = Color(230, 230, 230);

            return element | bgcolor(Color(0xff, 0, 0, 20)) | color(textColor) | bold;
        } else {
            return element | bgcolor(Color::Black) | color(Color::GrayLight);
        }
    };

    m_input_component = ftxui::Input(&user_input, "Enter message", opt) | borderRounded |
                        color(Color(0xff, 0x8f, 0xa6)) | size(HEIGHT, EQUAL, 3);
}

void ClientInterface::styleMessages() {
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

Component ClientInterface::createMainContainer() {
    std::function<bool(Event)> scrollCB = [this](Event event) {
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
           bgcolor(Color(0x0f, 0x00, 0x20)) | CatchEvent(scrollCB);
}

ClientInterface::ClientInterface() : m_screen(ScreenInteractive::Fullscreen()) {
    styleInput();
    styleMessages();
}

void ClientInterface::refresh() {
    m_screen.PostEvent(Event::Custom);
}

void ClientInterface::scrollDown() {
    m_scroll_offset = 1.0f;
}

Component ClientInterface::printMessage(const std::string &msg, const std::string &user,
                                        Decorator nameColor) {
    std::string str     = msg;
    auto        message = Renderer([str, user, nameColor] {
        return vbox({paragraph(std::format("[{}]", user)) | nameColor, paragraph(" " + str)}) |
               borderStyled(Color(0xff, 0x8f, 0xa6)) | xflex;
    });
    m_messages_container->Add(message);
    m_screen.PostEvent(Event::Custom);
    return message;
}

void ClientInterface::init() {
    auto cont = createMainContainer();
    m_input_component->TakeFocus();
    m_screen.Loop(cont);
}
