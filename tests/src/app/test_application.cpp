#include "catch2/catch.hpp"

#include <iostream>

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/common/thread.hpp"

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

SCENARIO("TheApplication Singleton", "[app][application]") {

//    SECTION("can be accessed anywhere in the code") { REQUIRE(TheApplication::get_arguments().argc == -1); }

//    SECTION("can be started (with exec) and shut down") {
//        int result = TheApplication::exec(); // does not block since no Windows are open
//        TheApplication::shutdown();
//        REQUIRE(result == EXIT_SUCCESS);
//    }

//    SECTION("can not be initialized more than once") {
//        TheApplication::AccessFor<Tester>::reinitialize();
//        REQUIRE_THROWS_AS(TheApplication::initialize(42, nullptr), TheApplication::StartupError);
//    }

//    SECTION("can not run exec more than once") {
//        TheApplication::exec();
//        REQUIRE_THROWS_AS(TheApplication::exec(), TheApplication::StartupError);
//    }

//    // Always reset the Application singleton when the test case has finished
//    TheApplication::AccessFor<Tester>::reinitialize();
}

NOTF_USING_NAMESPACE;
