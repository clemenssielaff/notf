#pragma once

#include <string>
#include <vector>

namespace signal {

/// \brief Tokenizes a string.
///
/// \param input        Input string.
/// \param delimiter    Delimiter character (is removed from the tokens).
///
/// \return String tokens.
inline std::vector<std::string> tokenize(const std::string& input, const char delimiter)
{
    std::vector<std::string> result;
    if (input.empty()) {
        return result;
    }
    std::string::size_type start_pos{ 0 };
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

/// \brief tokenize() overload to deal with (potentially nullptr) c-style character arrays.
///
/// \param input        Input string.
/// \param delimiter    Delimiter character.
///
/// \return String tokens.
inline std::vector<std::string> tokenize(const char* input, const char delimiter)
{
    if (!input) {
        return {};
    }
    return tokenize(std::string(input), delimiter);
}

/// \brief Tests if an input string starts with a given prefix.
/// \param input    Input string.
/// \param prefix   Prefix to test for.
/// \return True iff the first n input string characters match the prefix of size n.
///
inline bool starts_with(const std::string& input, const std::string& prefix)
{
    return (input.compare(0, prefix.size(), prefix) == 0);
}

} // namespace signal
