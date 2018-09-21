#include "catch2/catch.hpp"

#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("StringConsts", "[meta][stringtype]")
{
    constexpr StringConst test_string = "th/s_1s-A:T3st!";
    size_t constexpr_hash = test_string.get_hash();

    REQUIRE(test_string.get_size() == 15);
    REQUIRE(constexpr_hash == hash_string(test_string.c_str(), test_string.get_size()));

}
