#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* argv[])
{
    const int result = Catch::Session().run(argc, argv);
    return 0 ;
}
