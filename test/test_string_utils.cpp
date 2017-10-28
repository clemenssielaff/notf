#include "catch.hpp"

#include "common/string.hpp"
using notf::tokenize;

SCENARIO("strings can be tokenized", "[common][string]") {
	WHEN("a string with one or more delimiter characters is passed") {
		const char* simple_char = "hello-world";
		std::string simple_str  = simple_char;
		std::vector<std::string> simple_expected{"hello", "world"};

		const char* multi_char = "hello-world-or-whatever";
		std::string multi_str  = multi_char;
		std::vector<std::string> multi_expected{"hello", "world", "or", "whatever"};

		THEN("tokenize() will split the string into tokens, removing the delimiters") {
			REQUIRE(tokenize(simple_char, '-') == simple_expected);
			REQUIRE(tokenize(simple_str, '-') == simple_expected);
			REQUIRE(tokenize(multi_char, '-') == multi_expected);
			REQUIRE(tokenize(multi_str, '-') == multi_expected);
		}
	}

	WHEN("an empty string is passed") {
		const char* empty_char = "";
		std::string empty_str  = empty_char;

		THEN("the result is an empty vector") {
			REQUIRE(tokenize(empty_char, '-') == std::vector<std::string>());
			REQUIRE(tokenize(empty_str, '-') == std::vector<std::string>());
		}
	}

	WHEN("an nullptr c-style character array is passed") {
		const char* null_char = nullptr;

		THEN("the result is an empty vector") { REQUIRE(tokenize(null_char, '-') == std::vector<std::string>()); }
	}

	WHEN("a string with additional delimiters at the start- or end is passed") {
		const char* weird_char = "--hello--world-what-indeed----";
		std::string weird_str  = weird_char;
		std::vector<std::string> expected{"hello", "world", "what", "indeed"};

		THEN("tokenize() will trim the delimiters and proceed") {
			REQUIRE(tokenize(weird_char, '-') == expected);
			REQUIRE(tokenize(weird_str, '-') == expected);
		}
	}

	WHEN("a string without any delimiters is passed") {
		const char* no_delimiter_char = "helloworld";
		std::string no_delimiter_str  = no_delimiter_char;
		std::vector<std::string> expected{"helloworld"};

		THEN("tokenize() returns a vector with a single entry") {
			REQUIRE(tokenize(no_delimiter_char, '-') == expected);
			REQUIRE(tokenize(no_delimiter_str, '-') == expected);
		}
	}
}
