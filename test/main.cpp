#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "app/application.hpp"
#include "app/window.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/optional.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

namespace {
std::optional<WindowPtr> g_window;
}

NOTF_OPEN_NAMESPACE

WindowPtr notf_window()
{
    if (!g_window.has_value()) {
        notf_throw(internal_error, "Could not get NoTF Window for testing");
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
        Window::Args args;
        args.state = Window::State::MINIMIZED;
        g_window = Application::instance().create_window(args);
    }

    int result = Catch::Session().run(argc, argv);

    // shut down the application by closing the test window
    notf_window()->close();

    return result;
}
