#define CATCH_CONFIG_RUNNER
#include "test/catch.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
using namespace notf;

int main(int argc, char* argv[])
{
    set_log_level(LogMessage::LEVEL::NONE);
    Application::initialize(argc, argv);

    int result = Catch::Session().run(argc, argv);

    return result;
}
