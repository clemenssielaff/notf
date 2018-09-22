#include "catch2/catch.hpp"

#include "notf/meta/exception.hpp"

NOTF_USING_NAMESPACE;

namespace {

[[noreturn]] void throwing_no_msg() { NOTF_THROW(value_error); }

[[noreturn]] void throwing_with_msg() { NOTF_THROW(logic_error, "this is a {} message", "great"); }

} // namespace

SCENARIO("exception", "[meta][exception]")
{
    SECTION("simple exception throwing with the NOTF_THROW macro")
    {
        REQUIRE_THROWS_AS(throwing_no_msg(), value_error);
    }

    SECTION("NOTF_THROW supports formatted messages")
    {
        try {
            throwing_with_msg();
        }
        catch (const logic_error& error) {
            REQUIRE(std::string(error.what()) == std::string("this is a great message"));
        }
    }
}
