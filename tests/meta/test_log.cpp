#include "catch.hpp"

#include <iostream>

#include "notf/meta/log.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("the logger", "[meta][log]") {
    auto& the_logger = TheLogger::get();
    SECTION("assign log levels at run time") {
        REQUIRE(the_logger->level() == spdlog::level::trace);
        the_logger->set_level(spdlog::level::off);
        the_logger->trace("this will {} be printed", "NOT");
    }
    the_logger->set_level(spdlog::level::trace); // always set the level back
}

// SCENARIO("manual logger", "[meta][log]") {
//    SECTION("you can create additional loggers") {
//        TheLogger::Arguments logger_args;
//        logger_args.console_level = TheLogger::Level::OFF; // do not print to the console
//    }

//    SECTION("you can create loggers that write to a file") // ... but test them in a separate, slow scenario
//    {
//        TheLogger::Arguments logger_args;
//        logger_args.file_name = "test_log.txt";
//        TheLogger test_logger(logger_args);
//    }
//}

// SCENARIO("log to file", "[meta][log][slow]") {
//    SECTION("loggers can write to files as well") {
//        TheLogger::Arguments logger_args;
//        logger_args.file_name = "test_log.txt";
//        logger_args.console_level = TheLogger::Level::OFF; // do not print to the console
//        TheLogger test_logger(logger_args);

//        test_logger->trace("we can write {} out to a file", "this");
//    }
//}
