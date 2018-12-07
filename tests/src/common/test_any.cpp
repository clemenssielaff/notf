#include "catch2/catch.hpp"

#include "notf/common/any.hpp"
#include "notf/meta/real.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("fuzzy_any_cast allows any_casting with compatible types", "[common][any]") {
    SECTION("to integral") {
        REQUIRE(fuzzy_any_cast<int>(std::any(true)) == 1);
        REQUIRE(fuzzy_any_cast<int>(std::any(int8_t(4))) == 4);
        REQUIRE(fuzzy_any_cast<long>(std::any(int16_t(-400))) == -400);
        REQUIRE(fuzzy_any_cast<int>(std::any(int32_t(48564))) == 48564);
        REQUIRE(fuzzy_any_cast<long>(std::any(int64_t(9876534))) == 9876534);
        REQUIRE(fuzzy_any_cast<uint>(std::any(uint8_t(4))) == 4);
        REQUIRE(fuzzy_any_cast<ulong>(std::any(uint16_t(400))) == 400);
        REQUIRE(fuzzy_any_cast<uint>(std::any(uint32_t(48564))) == 48564);
        REQUIRE(fuzzy_any_cast<ulong>(std::any(uint64_t(9876534))) == 9876534);
        REQUIRE(fuzzy_any_cast<ulong>(std::any(uint64_t(9876534))) == 9876534);

        REQUIRE_THROWS_AS(fuzzy_any_cast<int>(std::any(None())), std::bad_any_cast);
    }

    SECTION("to floating pointer") {
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(true)), 1));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(485.f)), 485));
        REQUIRE(is_approx(fuzzy_any_cast<float>(std::any(68735.846)), 68735.846f));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(int8_t(4))), 4));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(int16_t(-400))), -400));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(int32_t(48564))), 48564));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(int64_t(9876534))), 9876534));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(uint8_t(4))), 4));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(uint16_t(400))), 400));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(uint32_t(48564))), 48564));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(uint64_t(9876534))), 9876534));
        REQUIRE(is_approx(fuzzy_any_cast<double>(std::any(uint64_t(9876534))), 9876534));

        REQUIRE_THROWS_AS(fuzzy_any_cast<float>(std::any(None())), std::bad_any_cast);
    }
}
