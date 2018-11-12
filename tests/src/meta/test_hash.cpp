#include "catch2/catch.hpp"

#include "notf/meta/hash.hpp"
#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("hash functions", "[meta][hash]")
{
    SECTION("hash is a variadic function")
    {
        const int int_v = 852758;
        const float float_v = 654.358435f;
        const bool bool_v = true;

        const size_t total_hash = hash(int_v, float_v, bool_v);
        REQUIRE(total_hash != hash(int_v));
        REQUIRE(total_hash != hash(float_v));
        REQUIRE(total_hash != hash(bool_v));
    }

    SECTION("constexpr string and runtime strings are hashed to the same value")
    {
        constexpr StringConst const_string = "this /s A T3st_!";
        constexpr size_t const_string_hash = hash_string(const_string.c_str(), const_string.get_size());
        static_assert(const_string_hash == const_string.get_hash());
        static_assert(const_string_hash != 0);

        const std::string runtime_string = const_string.c_str();
        REQUIRE(const_string_hash == hash_string(runtime_string.c_str(), runtime_string.size()));
        REQUIRE(const_string_hash == hash_string(std::string(const_string.c_str())));
    }

    SECTION("hash_mix is a function to improve hash functions with low entropy")
    {
        REQUIRE(hash_mix(uint(1)) != hash(uint(1)));
        REQUIRE(hash_mix(size_t(1)) != hash(size_t(1)));
    }
}
