#include "catch.hpp"

#include "notf/meta/hash.hpp"
#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("hash functions", "[meta][hash]") {
    SECTION("hash is a variadic function") {
        const int int_v = 852758;
        const float float_v = 654.358435f;
        const bool bool_v = true;
        const detail::HashID hash_id = detail::HashID::COLOR;

        const size_t total_hash = hash(int_v, float_v, bool_v, hash_id);
        REQUIRE(total_hash != hash(int_v));
        REQUIRE(total_hash != hash(float_v));
        REQUIRE(total_hash != hash(bool_v));
        REQUIRE(total_hash != hash(hash_id));
    }

    SECTION("the hash id is a simple number") {
        REQUIRE(hash(detail::HashID::SIZE2) == hash(to_number(detail::HashID::SIZE2)));
    }

    SECTION("constexpr string and runtime strings are hashed to the same value") {
        constexpr ConstString const_string = "this /s A T3st_!";
        constexpr size_t const_string_hash = hash_string(const_string.c_str(), const_string.get_size());
        static_assert(const_string_hash != 0);
        static_assert(const_string_hash == const_string.get_hash());

        const std::string runtime_string = const_string.c_str();
        REQUIRE(const_string_hash == hash_string(runtime_string.c_str(), runtime_string.size()));
        REQUIRE(const_string_hash == hash_string(std::string(const_string.c_str())));
    }

    SECTION("hash_mix is a function to improve hash functions with low entropy") {
        REQUIRE(hash_mix(uint(1)) != hash(uint(1)));
        REQUIRE(hash_mix(size_t(1)) != hash(size_t(1)));
    }

    SECTION("the magic hash number") { REQUIRE(detail::magic_hash_number<std::size_t>() == 0x9e3779b97f4a7c16); }
}
