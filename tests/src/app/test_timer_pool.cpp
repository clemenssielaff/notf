#include "catch2/catch.hpp"

#include <iostream>

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/common/thread.hpp"

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

SCENARIO("TheApplication Singleton", "[app][application]") {

    SECTION("no instance") {
        SECTION("Requesting TheApplication without a running instance will throw `SingletonError`") {
            REQUIRE_THROWS_AS(TheApplication()->get_arguments(), SingletonError);
        }
    }

    SECTION("valid instance") {
        TheApplication app(TheApplication::Arguments{});

        SECTION("can be accessed anywhere in the code") { REQUIRE(TheApplication()->get_arguments().argc == -1); }

        SECTION("can be started (with exec) and shut down") {
            int result = TheApplication()->exec(); // does not block since no Windows are open
            TheApplication()->shutdown();
            REQUIRE(result == EXIT_SUCCESS);
        }

        SECTION("can not run exec more than  once") {
            TheApplication()->exec();
            REQUIRE_THROWS_AS(TheApplication()->exec(), TheApplication::StartupError);
        }

        SECTION("you can test whether this is the UI thread from anywhere") {
            REQUIRE(this_thread::is_the_ui_thread());
            bool hopefully_not = true;
            Thread().run([&] { hopefully_not = this_thread::is_the_ui_thread(); });
            REQUIRE(!hopefully_not);
        }
    }
}

NOTF_USING_NAMESPACE;
