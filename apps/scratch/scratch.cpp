#include <iostream>

#include "notf/common/thread.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/app/application.hpp"
#include "notf/app/driver.hpp"
#include "notf/app/graph/slot.hpp"
#include "notf/app/graph/window.hpp"

NOTF_OPEN_NAMESPACE

int run_main(int argc, char* argv[]) {
    // initialize application
    TheApplication::Arguments arguments;
    arguments.argc = argc;
    arguments.argv = argv;
    TheApplication app(std::move(arguments));

    auto window1 = Window::create();

    Thread input_thread;
    input_thread.run([window = std::move(window1)]() mutable {
        Driver driver(std::move(window));
        {
            using namespace std::chrono_literals;
            using namespace notf::driver;
            this_thread::sleep_for(2s);
            //            driver << "abc";
        }
    });
    auto result = app->exec();
    input_thread.join();
    return result;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
