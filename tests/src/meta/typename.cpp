#include "catch.hpp"

#include "notf/meta/typename.hpp"

NOTF_USING_META_NAMESPACE;

class TestClass75 {};

SCENARIO("NoTF has a mechanism to demangle C++ type names", "[utils][type_name]")
{
    const std::string class_name1 = type_name<TestClass75>();
    const std::string class_name2 = type_name(TestClass75{});
    const std::string class_name3 = type_name(typeid(TestClass75));

    const std::string expected = "TestClass75";
    REQUIRE(class_name1 == expected);
    REQUIRE(class_name2 == expected);
    REQUIRE(class_name3 == expected);
}
