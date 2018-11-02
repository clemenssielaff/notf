#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "notf/app/application.hpp"

int main(int argc, char* argv[])
{
    NOTF_USING_NAMESPACE;

    // disable logger
    TheApplication::Args application_arguments;
    application_arguments.logger_arguments.console_level = Logger::Level::OFF;
    TheApplication::initialize(application_arguments);

    return Catch::Session().run(argc, argv);
}
