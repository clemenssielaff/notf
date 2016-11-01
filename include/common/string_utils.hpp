#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace notf {

/// @brief Tokenizes a string.
///
/// @param input        Input string.
/// @param delimiter    Delimiter character (is removed from the tokens).
///
/// @return String tokens.
inline std::vector<std::string> tokenize(const std::string& input, const char delimiter)
{
    std::vector<std::string> result;
    if (input.empty()) {
        return result;
    }
    std::string::size_type start_pos{0};
    std::string::size_type end_pos = input.find_first_of(delimiter, start_pos);
    while (end_pos != std::string::npos) {
        if (auto len = end_pos - start_pos) {
            result.emplace_back(input.substr(start_pos, len));
        }
        start_pos = end_pos + 1;
        end_pos = input.find_first_of(delimiter, start_pos);
    }
    std::string last_entry = input.substr(start_pos, end_pos - start_pos);
    if (!last_entry.empty()) {
        result.emplace_back(std::move(last_entry));
    }
    return result;
}

/// @brief tokenize() overload to deal with (potentially nullptr) c-style character arrays.
///
/// @param input        Input string.
/// @param delimiter    Delimiter character.
///
/// @return String tokens.
inline std::vector<std::string> tokenize(const char* input, const char delimiter)
{
    if (!input) {
        return {};
    }
    return tokenize(std::string(input), delimiter);
}

/// @brief Tests if an input string starts with a given prefix.
/// @param input    Input string.
/// @param prefix   Prefix to test for.
/// @return True iff the first n input string characters match the prefix of size n.
///
inline bool starts_with(const std::string& input, const std::string& prefix)
{
    return (input.compare(0, prefix.size(), prefix) == 0);
}

/// @brief Case-insensitive string comparison.
/// @return True iff both strings are the same in lower case letters.
///
inline bool icompare(const std::string& left, const std::string& right)
{
    if (left.length() == right.length()) {
        return std::equal(right.begin(), right.end(), left.begin(), [](auto a, auto b) -> bool {
            return std::tolower(a) == std::tolower(b);
        });
    }
    else {
        return false;
    }
}

/// @brief Case-insensitive test, if the input string starts with a given prefix.
/// @param input    Input string.
/// @param prefix   Prefix to test for.
/// @return True iff the first n input string characters match the prefix of size n.
///
inline bool istarts_with(const std::string& input, const std::string& prefix)
{
    std::string relevant_input = input.substr(0, prefix.length());
    std::transform(relevant_input.begin(), relevant_input.end(), relevant_input.begin(), [](auto c) {
        return std::tolower(c);
    });
    return relevant_input.compare(0, prefix.length(), prefix) == 0;
}

/// \brief String Formatting function using std::snprinftf.
///
/// For details on the format string see: http://en.cppreference.com/w/cpp/io/c/fprintf
/// Adapted from: http://stackoverflow.com/a/26221725/3444217
template <typename... Args>
std::string string_format(const std::string& format, Args... args)
{
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buffer(new char[size]);
    std::snprintf(buffer.get(), size, format.c_str(), args...);
    return std::string(buffer.get(), buffer.get() + size - 1); // We don't want the '\0' inside
}

} // namespace notf
