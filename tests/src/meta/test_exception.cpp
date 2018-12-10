#include "catch2/catch.hpp"

#include "notf/common/string.hpp"
#include "notf/meta/exception.hpp"

NOTF_USING_NAMESPACE;

namespace {

NOTF_NORETURN void throwing_no_msg() { NOTF_THROW(ValueError); }

NOTF_NORETURN void throwing_with_msg() { NOTF_THROW(LogicError, "this is a {} message", "great"); }

} // namespace

SCENARIO("exception", "[meta][exception]") {
    SECTION("simple exception throwing with the NOTF_THROW macro") { REQUIRE_THROWS_AS(throwing_no_msg(), ValueError); }

    SECTION("NOTF_THROW supports formatted messages") {
        try {
            throwing_with_msg();
        }
        catch (const LogicError& error) {
            REQUIRE(std::string(error.what()) == std::string("(LogicError) this is a great message"));
        }
    }

    SECTION("exceptions can tell their origin") {
        try {
            throwing_with_msg();
        }
        catch (const LogicError& error) {
            REQUIRE(error.get_line() == 12);
            REQUIRE(std::string(error.get_file()) == "test_exception.cpp");
#ifdef NOTF_MSVC
            REQUIRE(ends_with(error.get_function(), "throwing_with_msg"));
#else
            REQUIRE(ends_with(error.get_function(), "throwing_with_msg()"));
#endif
        }
    }
}
