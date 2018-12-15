#include "catch2/catch.hpp"

#include "notf/common/thread.hpp"

#include "notf/app/application.hpp"
#include "notf/app/driver.hpp"
#include "notf/app/window.hpp"

#include "test/app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Application Driver", "[app][driver]") {
    TheApplication app(TheApplication::Arguments{});
    auto window1 = Window::create();
    Thread input_thread;
    input_thread.run([window = std::move(window1)]() mutable {
        using namespace notf::driver;
        Driver driver(window);
        driver << "hello" << Mouse(LEFT);
        REQUIRE(driver.get_window() == window);
        TheApplication()->shutdown();
    });
    app->exec();
    input_thread.join();
}