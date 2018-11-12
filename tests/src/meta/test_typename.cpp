#include "catch2/catch.hpp"

#include "notf/meta/typename.hpp"

NOTF_USING_NAMESPACE;

class TestClass75 {};

SCENARIO("demangle C++ type names", "[meta][type_name]")
{
    const std::string class_name1 = type_name<TestClass75>();
    const std::string class_name2 = type_name(TestClass75{});
    const std::string class_name3 = type_name(typeid(TestClass75));

#ifdef NOTF_MSVC
    const std::string expected = "class TestClass75";
#else
    const std::string expected = "TestClass75";
#endif
    REQUIRE(class_name1 == expected);
    REQUIRE(class_name2 == expected);
    REQUIRE(class_name3 == expected);

    const std::string invalid_name = "notvalid";
    const std::string invalid_result = detail::demangle_type_name(invalid_name.c_str());
    REQUIRE(invalid_result == invalid_name);
}
