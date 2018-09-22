#include "catch2/catch.hpp"

#include "notf/common/vector.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("vector", "[common][vector]")
{
    SECTION("remove all occurrences of 'element' from a vector")
    {
        std::vector<int> vec = {0, 1, 0, 3, 4, 0, 5, 6, 7, 0, 0};
        const std::vector<int> expected = {1, 3, 4, 5, 6,  7};

        remove_all(vec, 0);
        REQUIRE(vec == expected);
    }
}
