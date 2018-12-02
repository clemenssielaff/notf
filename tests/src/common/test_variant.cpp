#include "catch2/catch.hpp"

#include "notf/common/variant.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("variant", "[common][variant]") {

    SECTION("get_first_variant_index") {
        using MyVariant = std::variant<int, float, bool>;
        REQUIRE(get_first_variant_index<float, MyVariant>() == 1);
    }

    SECTION("is_one_of_variant") {
        using MyVariant = std::variant<int, float, bool>;
        REQUIRE(is_one_of_variant<float, MyVariant>());
        REQUIRE(!is_one_of_variant<None, MyVariant>());
    }
}
