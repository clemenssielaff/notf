#include "catch.hpp"
#include "test_utils.hpp"

using namespace notf;

TEST_CASE("Pack and unpack two halfes", "[common][half]")
{
    const half a(random_number<float>());
    const half b(random_number<float>());

    auto pack = packHalfs(a, b);
    auto unpacked = unpackHalfs(pack);

    const half c = unpacked.first;
    const half d = unpacked.second;

    REQUIRE(float(a) == approx(float(c), 0.f));
    REQUIRE(float(b) == approx(float(d), 0.f));
}
