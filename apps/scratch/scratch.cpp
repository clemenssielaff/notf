#include <atomic>
#include <iostream>
#include <thread>

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

#include "notf/common/thread.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/app/slot.hpp"

NOTF_OPEN_NAMESPACE

int run_main(int argc, char* argv[]) {
    // initialize application
    TheApplication::Arguments arguments;
    arguments.argc = argc;
    arguments.argv = argv;
    TheApplication app(std::move(arguments));

    auto window1 = Window::create();
//    auto window2 = Window::create();

    auto result = app->exec();
    return result;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
