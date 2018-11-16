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
std::vector<std::string> split(const std::string& input, char delimiter);

/// tokenize() overload to deal with (potentially nullptr) c-style character arrays.
/// @param input         Input string.
/// @param delimiter     Delimiter character.
/// @return              String tokens.
inline std::vector<std::string> split(const char* input, const char delimiter)
{
    return (static_cast<bool>(input) ? split(std::string(input), delimiter) : std::vector<std::string>());
}

/// Remove all spaces on the left of the string.
/// @param str  String to modify.
inline void ltrim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) { return !std::isspace(ch); }));
}
inline std::string ltrim_copy(std::string str)
{
    ltrim(str);
    return str;
}

/// Remove all spaces on the right of the string.
/// @param str  String to modify.
inline void rtrim(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) { return !std::isspace(ch); }).base(), str.end());
}
inline std::string rtrim_copy(std::string str)
{
    rtrim(str);
    return str;
}

/// Remove all spaces from both ends of the string.
/// @param str  String to modify.
inline void trim(std::string& str)
{
    ltrim(str);
    rtrim(str);
}
inline std::string trim_copy(std::string str)
{
    trim(str);
    return str;
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

/// Get the length of a c string literal at compile time (excluding the null terminator).
constexpr inline size_t cstring_length(const char* str)
{
    size_t i = 0;
    while (str[i] != '\0') {
        ++i;
    }
    return i;
}

NOTF_CLOSE_NAMESPACE
