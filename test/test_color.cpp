#include "catch.hpp"

#include "common/color.hpp"
using namespace notf;

SCENARIO("Colors can be constructed from strings", "[common][color][string]")
{
    WHEN("a valid string is passed")
    {
        std::string str_1 = "2db223";    // rgb(45, 178,  35, 255)
        std::string str_2 = "#2f0333";   // rgb(47,   3,  51, 255)
        std::string str_3 = "#C0D9DB";   // rgb(192, 217, 219, 255)
        std::string str_4 = "#1c1B0d";   // rgb(28,  27,  13, 255)
        std::string str_5 = "#adadad80"; // rgb(173, 173, 173, 128)
        std::string str_6 = "#3062a355"; // rgb(48,  98, 163,  85)

        THEN("Color(string) will interpret it correctly")
        {
            REQUIRE(Color(str_1) == Color(45, 178, 35, 255));
            REQUIRE(Color(str_2) == Color(47, 3, 51, 255));
            REQUIRE(Color(str_3) == Color(192, 217, 219, 255));
            REQUIRE(Color(str_4) == Color(28, 27, 13, 255));
            REQUIRE(Color(str_5) == Color(173, 173, 173, 128));
            REQUIRE(Color(str_6) == Color(48, 98, 163, 85));
        }
    }

    WHEN("an invalid string is passed")
    {
        std::string wrong_1 = "#0d9db";
        std::string wrong_2 = "0d9db";
        std::string wrong_3 = "#2f03338";
        std::string wrong_4 = "c0d9db1";
        std::string wrong_5 = "#2f0x33";

        THEN("Color(string) will throw an error")
        {
            Color _;
            REQUIRE_THROWS(_ = Color(wrong_1));
            REQUIRE_THROWS(_ = Color(wrong_2));
            REQUIRE_THROWS(_ = Color(wrong_3));
            REQUIRE_THROWS(_ = Color(wrong_4));
            REQUIRE_THROWS(_ = Color(wrong_5));
        }
    }
}

SCENARIO("Colors can be constructed from HSL values", "[common][color]")
{
    WHEN("a color is constructed from HSL value")
    {
        Color hsl_1 = Color::from_hsl(pi<float>() / 4, 0.33f, 0.71f);

        THEN("it has the correct RGB value")
        {
            REQUIRE(hsl_1.to_string() == Color(205, 193, 157).to_string());
        }
    }
}
