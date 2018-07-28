#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <optional>

#include "app/application.hpp"
#include "app/window.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE;

namespace {
std::optional<WindowPtr> g_window;
}

NOTF_OPEN_NAMESPACE

WindowPtr notf_window()
{
    if (!g_window.has_value()) {
        NOTF_THROW(internal_error, "Could not get NoTF Window for testing");
    }
    return g_window.value();
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[])
{
    set_log_level(LogMessage::LEVEL::NONE);

    // initialize the application
    Application::initialize(argc, argv);

    { // initialize the window
        Window::Settings window_settings;
        window_settings.state = Window::Settings::State::MINIMIZED;
        g_window = Application::instance().create_window(window_settings);
    }

    int result = Catch::Session().run(argc, argv);

    { // shut down the application by closing the test window
        WindowPtr window = notf_window();
        g_window.value().reset();
        window->close();
    }

    return result;
}
