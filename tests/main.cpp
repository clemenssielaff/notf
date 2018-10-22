#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "notf/meta/log.hpp"

int main(int argc, char* argv[])
{
    // initialize logger
    notf::TheLogger::Args logger_args;
    logger_args.console_level = notf::TheLogger::Level::OFF;
    notf::TheLogger::initialize(logger_args); // move test Logger initialization into Application, once we have one

    return Catch::Session().run(argc, argv);
}
