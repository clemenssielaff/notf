#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "common/log.hpp"
using namespace notf;

int main(int argc, char* argv[]) {
	set_log_level(LogMessage::LEVEL::NONE);

	int result = Catch::Session().run(argc, argv);

	return result;
}
