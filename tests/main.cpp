#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "notf/meta/log.hpp"

int main(int argc, char* argv[]) {
    NOTF_USING_NAMESPACE;

    // disable logger
    TheLogger::Arguments args;
    args.console_level = Logger::Level::OFF;
    TheLogger::initialize(args);

    return Catch::Session().run(argc, argv);
}
