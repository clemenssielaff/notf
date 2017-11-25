#include "catch.hpp"

#include "common/string.hpp"
using notf::tokenize;

TEST_CASE("tokenize strings", "[common][string]")
{
    SECTION("a string with one or more delimiter characters is passed")
    {
        const char* simple_char = "hello-world";
        std::string simple_str  = simple_char;
        std::vector<std::string> simple_expected{"hello", "world"};

        const char* multi_char = "hello-world-or-whatever";
        std::string multi_str  = multi_char;
        std::vector<std::string> multi_expected{"hello", "world", "or", "whatever"};

        REQUIRE(tokenize(simple_char, '-') == simple_expected);
        REQUIRE(tokenize(simple_str, '-') == simple_expected);
        REQUIRE(tokenize(multi_char, '-') == multi_expected);
        REQUIRE(tokenize(multi_str, '-') == multi_expected);
    }

    SECTION("an empty string results in an empty vector")
    {
        const char* empty_char = "";
        std::string empty_str  = empty_char;

        REQUIRE(tokenize(empty_char, '-') == std::vector<std::string>());
        REQUIRE(tokenize(empty_str, '-') == std::vector<std::string>());
    }

    SECTION("an nullptr c-style character array is passed, the result in an empty vector")
    {
        const char* null_char = nullptr;

        REQUIRE(tokenize(null_char, '-') == std::vector<std::string>());
    }

    SECTION("additional delimiters at the start- will be trimmed")
    {
        const char* weird_char = "--hello--world-what-indeed----";
        std::string weird_str  = weird_char;
        std::vector<std::string> expected{"hello", "world", "what", "indeed"};

        REQUIRE(tokenize(weird_char, '-') == expected);
        REQUIRE(tokenize(weird_str, '-') == expected);
    }

    SECTION("a string without any delimiters results in a vector with a single entry")
    {
        const char* no_delimiter_char = "helloworld";
        std::string no_delimiter_str  = no_delimiter_char;
        std::vector<std::string> expected{"helloworld"};

        REQUIRE(tokenize(no_delimiter_char, '-') == expected);
        REQUIRE(tokenize(no_delimiter_str, '-') == expected);
    }
}
