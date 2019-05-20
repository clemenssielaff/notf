#include "catch.hpp"

#include "notf/app/application.hpp"
#include "notf/app/event_handler.hpp"
#include "notf/app/graph/window.hpp"

#include "test/app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Window", "[app][window]") {
    TheApplication app(test_app_arguments());

    SECTION("open and close an empty window") {
        auto window = Window::create();
        TheEventHandler()->schedule([&window] { window->call<Window::to_close>(); });
        TheApplication()->exec();
    }

    SECTION("every Window has an associated GLFW window") {
        auto window = Window::create();
        REQUIRE(window->get_glfw_window() != nullptr);
    }

    SECTION("Windows may only be created from the UI thread") {
        Thread other;
        other.run([] { REQUIRE_THROWS_AS(Window::create(), ThreadError); });
    }

    SECTION("Windows created from the main thread are set up immediately, those from the event thread are deferred") {
        Window::Arguments first_args;
        first_args.title = "first";
        auto first = Window::create(first_args);
        TheEventHandler()->schedule([&first] {

            Window::Arguments second_args;
            second_args.title = "second";
            auto second = Window::create(second_args);
            first->call<Window::to_close>();
            second->call<Window::to_close>();
        });
        TheApplication()->exec();
    }
}

NOTF_USING_NAMESPACE;
