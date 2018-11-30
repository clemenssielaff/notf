#include "catch2/catch.hpp"

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Window", "[app][window]") {

    SECTION("open a close an empty window") {
        TheApplication app(TheApplication::Arguments{});
        auto window = Window::create();
        window.call<Window::to_close>();
    }
}

NOTF_USING_NAMESPACE;
