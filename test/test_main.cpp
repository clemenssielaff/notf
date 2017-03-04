#define CATCH_CONFIG_RUNNER
#include "test/catch.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
using namespace notf;

int main(int argc, char* argv[])
{
    set_log_level(LogMessage::LEVEL::NONE);

    ApplicationInfo info;
    info.argc = argc;
    info.argv = argv;
    Application::initialize(info);

    int result = Catch::Session().run(argc, argv);

    return result;
}
