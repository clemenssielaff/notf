#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "test/catch.hpp"

#include "common/string_utils.hpp"

TEST_CASE("Tokenize", "[tokenize]")
{
    const char* good_char = "derbe-aufs-maul-indeed";
    const char* bad_char = "--derbe--aufs-maul-indeed----";
    std::string good_str = good_char;
    std::string bad_str = bad_char;
    std::vector<std::string> expected{ "derbe", "aufs", "maul", "indeed" };
    REQUIRE(tokenize("derbe-aufs-maul-indeed", '-') == expected);
    REQUIRE(tokenize(good_char, '-') == expected);
    REQUIRE(tokenize(bad_char, '-') == expected);
    REQUIRE(tokenize(good_str, '-') == expected);
    REQUIRE(tokenize(bad_str, '-') == expected);
    REQUIRE(tokenize("", '-').empty());
    REQUIRE(tokenize(std::string(), '-').empty());
}
