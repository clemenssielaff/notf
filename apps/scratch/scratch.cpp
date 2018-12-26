#include <iostream>

#include "notf/common/thread.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/app/application.hpp"
#include "notf/app/driver.hpp"
#include "notf/app/graph/slot.hpp"
#include "notf/app/graph/window.hpp"

#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

int run_main(int argc, char* argv[]) {
    // initialize application
    TheApplication::Arguments arguments;
    arguments.argc = argc;
    arguments.argv = argv;
    TheApplication app(std::move(arguments));

    WindowHandle window = Window::create();
    SceneHandle scene = window->set_scene<WidgetScene>();

    const int result = app->exec();
    return result;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
