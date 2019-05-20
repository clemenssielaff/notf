#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "notf/meta/log.hpp"

int main(int argc, char* argv[]) {
    NOTF_USING_NAMESPACE;

    // disable console output of the logger
    TheLogger::Arguments args;
    args.console_level = TheLogger::Level::OFF;
    TheLogger::initialize(args);

    const auto result = Catch::Session().run(argc, argv);
    return result;
}
