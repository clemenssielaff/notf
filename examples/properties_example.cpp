#include "renderthread_example.hpp"

#include "app/application.hpp"
#include "app/window.hpp"

NOTF_USING_NAMESPACE

int properties_main(int argc, char* argv[])
{
    Application::initialize(argc, argv);
    auto& app = Application::instance();

    Window::Args window_args;
    window_args.icon = "notf.png";
    auto window = Window::create(window_args);

    return app.exec();
}
