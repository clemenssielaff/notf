#pragma once

#include <string>
#include <vector>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// string helper functions ===========================================================================================//

/// Tokenizes a string.
/// @param input         Input string.
/// @param delimiter     Delimiter character (is removed from the tokens).
/// @return              String tokens.
std::vector<std::string> tokenize(const std::string& input, char delimiter);

/// tokenize() overload to deal with (potentially nullptr) c-style character arrays.
/// @param input         Input string.
/// @param delimiter     Delimiter character.
/// @return              String tokens.
inline std::vector<std::string> tokenize(const char* input, const char delimiter)
{
    return (static_cast<bool>(input) ? tokenize(std::string(input), delimiter) : std::vector<std::string>());
}

/// Tests if a string ends starts a given prefix.
/// @param input     Input string.
/// @param prefix    Prefix to test for.
/// @return True if the first n input string characters match the prefix of size n.
inline bool starts_with(const std::string& input, const std::string& prefix)
{
    return (input.compare(0, prefix.size(), prefix) == 0);
}

/// Case insensitive test if a string starts with a given prefix.
/// @param input     Input string.
/// @param prefix    Prefix to test for.
/// @return True iff the first n input string characters match the prefix of size n.
bool istarts_with(const std::string& input, const std::string& prefix);

/// Tests if a string ends with a given postfix.
/// @param input     Input string.
/// @param postfix   Postfix to test for.
/// @return True if the last n input string characters match the postfix of size n.
bool ends_with(const std::string& input, const std::string& postfix);

/// Case insensitive test if a string ends with a given postfix.
/// @param input     Input string.
/// @param postfix   Postfix to test for.
/// @return True iff the last n input string characters match the postfix of size n.
bool iends_with(const std::string& input, const std::string& postfix);

/// Case-insensitive string comparison.
/// @return  True iff both strings are the same in lower case letters.
bool icompare(const std::string& left, const std::string& right);

/// Levenshtein "string distance" algorithm.
/// Originally from
/// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
/// @param s1    First string.
/// @param s2    Second string.
/// @return      Number of additions, modifications or removals to get from s1 to s2.
size_t levenshtein_distance(const std::string& s1, const std::string& s2);

/// @{
/// Joins a vector of strings into a single string, optionally inserting a delimiter in between.
/// @param vec          Vector of strings to join.
/// @param delimiter    Delimiter inserted in between the vector items.
std::string join(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end,
                 const std::string& delimiter = "");
inline std::string join(const std::vector<std::string>& vec, const std::string& delimiter = "")
{
    return join(vec.cbegin(), vec.cend(), delimiter);
}
/// @}

/// Get the length of a c string literal at compile time.
constexpr size_t string_length(const char* str)
{
    size_t i = 0;
    while (str[i] != '\0') {
        ++i;
    }
    return i;
}

NOTF_CLOSE_NAMESPACE
