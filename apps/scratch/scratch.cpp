#include <atomic>
#include <iostream>
#include <thread>

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

int run_main(int argc, char* argv[])
{
    { // initialize application
        TheApplication::Args arguments;
        arguments.argc = argc;
        arguments.argv = argv;
        TheApplication::initialize(arguments);
    }

    auto window = Window::create();
    return TheApplication::exec();
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
