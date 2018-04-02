#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "app/application.hpp"
#include "app/window.hpp"
#include "common/exception.hpp"
#include "common/optional.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

namespace {
std::optional<std::reference_wrapper<Window>> g_window_reference;
}

Window& notf_window()
{
    if (!g_window_reference.has_value()) {
        notf_throw(internal_error, "Could not get NoTF Window for testing");
    }
    return g_window_reference.value();
}

int main(int argc, char* argv[])
{
    // initialize the application and the test window
    Application::initialize(argc, argv);
    g_window_reference = Application::instance().create_window();

    int result = Catch::Session().run(argc, argv);

    // shut down the application by closing the test window
    notf_window().close();

    return result;
}
