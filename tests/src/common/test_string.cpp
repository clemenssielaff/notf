#include "catch2/catch.hpp"

#include "notf/common/string.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("split strings", "[common][string]") {
    SECTION("a string with one or more delimiter characters is passed") {
        const char* simple_char = "hello-world";
        std::string simple_str = simple_char;
        std::vector<std::string> simple_expected{"hello", "world"};

        const char* multi_char = "hello-world-or-whatever";
        std::string multi_str = multi_char;
        std::vector<std::string> multi_expected{"hello", "world", "or", "whatever"};

        REQUIRE(split(simple_char, '-') == simple_expected);
        REQUIRE(split(simple_str, '-') == simple_expected);
        REQUIRE(split(multi_char, '-') == multi_expected);
        REQUIRE(split(multi_str, '-') == multi_expected);
    }

    SECTION("an empty string results in an empty vector") {
        const char* empty_char = "";
        std::string empty_str = empty_char;

        REQUIRE(split(empty_char, '-') == std::vector<std::string>());
        REQUIRE(split(empty_str, '-') == std::vector<std::string>());
    }

    SECTION("an nullptr c-style character array is passed, the result in an empty vector") {
        const char* null_char = nullptr;

        REQUIRE(split(null_char, '-') == std::vector<std::string>());
    }

    SECTION("additional delimiters at the start- will be trimmed") {
        const char* weird_char = "--hello--world-what-indeed----";
        std::string weird_str = weird_char;
        std::vector<std::string> expected{"hello", "world", "what", "indeed"};

        REQUIRE(split(weird_char, '-') == expected);
        REQUIRE(split(weird_str, '-') == expected);
    }

    SECTION("a string without any delimiters results in a vector with a single entry") {
        const char* no_delimiter_char = "helloworld";
        std::string no_delimiter_str = no_delimiter_char;
        std::vector<std::string> expected{"helloworld"};

        REQUIRE(split(no_delimiter_char, '-') == expected);
        REQUIRE(split(no_delimiter_str, '-') == expected);
    }
}

SCENARIO("string trimming", "[common][string]") {
    SECTION("rtrim in place") {
        std::string value = " Hallo Welt       ";
        rtrim(value);
        REQUIRE(value == " Hallo Welt");
    }
    SECTION("rtrim copy") {
        const std::string value = " Hallo Welt       ";
        const std::string copy = rtrim_copy(value);
        REQUIRE(copy == " Hallo Welt");
    }
    SECTION("ltrim in place") {
        std::string value = "    Hallo Welt ";
        ltrim(value);
        REQUIRE(value == "Hallo Welt ");
    }
    SECTION("ltrim copy") {
        const std::string value = "    Hallo Welt ";
        const std::string copy = ltrim_copy(value);
        REQUIRE(copy == "Hallo Welt ");
    }
    SECTION("trim in place") {
        std::string value = "    Hallo  Welt        ";
        trim(value);
        REQUIRE(value == "Hallo  Welt");
    }
    SECTION("ltrim copy") {
        const std::string value = "    Hallo  Welt        ";
        const std::string copy = trim_copy(value);
        REQUIRE(copy == "Hallo  Welt");
    }
}

SCENARIO("string startwith / endwith", "[common][string]") {
    SECTION("startwith case sensitive") {
        REQUIRE(starts_with(std::string(), ""));
        REQUIRE(starts_with(std::string("hello world"), "hello"));
        REQUIRE(starts_with(std::string(" derb"), " d"));
        REQUIRE(starts_with(std::string("what"), "w"));

        REQUIRE(!starts_with(std::string(), "anything"));
        REQUIRE(!starts_with(std::string("hello world"), "goodbye"));
        REQUIRE(!starts_with(std::string("hello world"), "hell "));
        REQUIRE(!starts_with(std::string("hello world"), "Hello"));
        REQUIRE(!starts_with(std::string(" derb"), "underb"));
    }

    SECTION("startwith case insensitive") {
        REQUIRE(istarts_with(std::string(), ""));
        REQUIRE(istarts_with(std::string("hello world"), "hello"));
        REQUIRE(istarts_with(std::string("hello world"), "Hello"));
        REQUIRE(istarts_with(std::string(" derb"), " d"));
        REQUIRE(istarts_with(std::string(" derb"), " D"));
        REQUIRE(istarts_with(std::string("what"), "w"));
        REQUIRE(istarts_with(std::string("what"), "W"));

        REQUIRE(!istarts_with(std::string(), "anything"));
        REQUIRE(!istarts_with(std::string("hello world"), "goodbye"));
        REQUIRE(!istarts_with(std::string("hello world"), "hell "));
        REQUIRE(!istarts_with(std::string(" derb"), "underb"));
    }

    SECTION("endswith case sensitive") {
        REQUIRE(ends_with(std::string(), ""));
        REQUIRE(ends_with(std::string("this is the end"), "end"));
        REQUIRE(ends_with(std::string("very derbe "), " derbe "));
        REQUIRE(ends_with(std::string("very derbe "), " "));

        REQUIRE(!ends_with(std::string(), "anything"));
        REQUIRE(!ends_with(std::string("this is the end"), "end?"));
        REQUIRE(!ends_with(std::string("hello world"), "worlD"));
        REQUIRE(!ends_with(std::string("hello world"), "World"));
        REQUIRE(!ends_with(std::string("very derbe"), "very derbe "));
    }

    SECTION("endswith case insensitive") {
        REQUIRE(iends_with(std::string(), ""));
        REQUIRE(iends_with(std::string("this is the end"), "end"));
        REQUIRE(iends_with(std::string("very derbe "), " derbe "));
        REQUIRE(iends_with(std::string("very derbe "), " "));
        REQUIRE(iends_with(std::string("hello world"), "worlD"));
        REQUIRE(iends_with(std::string("hello world"), "World"));

        REQUIRE(!iends_with(std::string(), "anything"));
        REQUIRE(!iends_with(std::string("this is the end"), "end?"));
        REQUIRE(!iends_with(std::string("very derbe"), "very derbe "));
    }

    SECTION("case insensitive comparison") {
        REQUIRE(icompare(std::string(), ""));
        REQUIRE(icompare(std::string("jup"), "jup"));
        REQUIRE(icompare(std::string("jup"), "JUP"));
        REQUIRE(icompare(std::string("jup"), "Jup"));
        REQUIRE(icompare(std::string("jup"), "JUp"));
        REQUIRE(icompare(std::string("jup"), "JuP"));
        REQUIRE(icompare(std::string("jup"), "jUP"));
        REQUIRE(icompare(std::string("jup"), "juP"));

        REQUIRE(!icompare(std::string("jup"), "juPn"));
        REQUIRE(!icompare(std::string("jup"), "jU"));
        REQUIRE(!icompare(std::string("jup"), "something else"));
        REQUIRE(!icompare(std::string(""), "anYThInG"));
    }

    SECTION("join strings") {
        std::vector<std::string> vec = {"hello", "world", ",", "what's", "up?"};
        const std::string result = join(vec, " ");
        REQUIRE(result == "hello world , what's up?");
        REQUIRE(join(std::vector<std::string>{}, "DELIMITER") == "");
        REQUIRE(join(std::vector<std::string>{"nojoinhere"}, "DELIMITER") == "nojoinhere");
        REQUIRE(join(std::vector<std::string>{"", "", ""}, "") == "");
        REQUIRE(join(std::vector<std::string>{"", "", ""}, "-") == "--");
        REQUIRE(join(std::vector<std::string>{"-", "-", "-"}, "") == "---");
    }

    SECTION("length of c strings") {
        const std::string test = "test";
        REQUIRE(cstring_length(test.c_str()) == 4);
    }
}
