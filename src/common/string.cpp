#include "common/string.hpp"

#include <algorithm>
#include <numeric>

namespace notf {

std::vector<std::string> tokenize(const std::string& input, const char delimiter)
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
        end_pos   = input.find_first_of(delimiter, start_pos);
    }
    std::string last_entry = input.substr(start_pos, end_pos - start_pos);
    if (!last_entry.empty()) {
        result.emplace_back(std::move(last_entry));
    }
    return result;
}

bool istarts_with(const std::string& input, const std::string& prefix)
{
    const auto input_size  = input.size();
    const auto prefix_size = prefix.size();
    if (prefix_size > input_size) {
        return false;
    }
    std::string relevant_input = input.substr(0, prefix.length());
    std::transform(relevant_input.begin(), relevant_input.end(), relevant_input.begin(),
                   [](auto c) { return std::tolower(c); });
    std::string prefix_lower = prefix;
    std::transform(prefix_lower.begin(), prefix_lower.end(), prefix_lower.begin(),
                   [](auto c) { return std::tolower(c); });
    return relevant_input.compare(0, prefix.length(), prefix) == 0;
}

bool ends_with(const std::string& input, const std::string& postfix)
{
    const auto input_size   = input.size();
    const auto postfix_size = postfix.size();
    if (postfix_size > input_size) {
        return false;
    }
    return (input.compare(input_size - postfix_size, postfix_size, postfix) == 0);
}

bool iends_with(const std::string& input, const std::string& postfix)
{
    const auto input_size   = input.size();
    const auto postfix_size = postfix.size();
    if (postfix_size > input_size) {
        return false;
    }
    std::string relevant_input = input.substr(input_size - postfix_size, input_size);
    std::transform(relevant_input.begin(), relevant_input.end(), relevant_input.begin(),
                   [](auto c) { return std::tolower(c); });
    std::string postfix_lower = postfix;
    std::transform(postfix_lower.begin(), postfix_lower.end(), postfix_lower.begin(),
                   [](auto c) { return std::tolower(c); });
    return (relevant_input.compare(0, postfix_size, postfix_lower) == 0);
}

bool icompare(const std::string& left, const std::string& right)
{
    if (left.length() == right.length()) {
        return std::equal(right.begin(), right.end(), left.begin(),
                          [](auto a, auto b) -> bool { return std::tolower(a) == std::tolower(b); });
    }
    else {
        return false;
    }
}

size_t levenshtein_distance(const std::string& s1, const std::string& s2)
{
    size_t s1len = s1.size();
    size_t s2len = s2.size();

    auto column_start = static_cast<decltype(s1len)>(1);

    auto column = new decltype(s1len)[s1len + 1];
    std::iota(column + column_start, column + s1len + 1, column_start);

    for (auto x = column_start; x <= s2len; x++) {
        column[0]          = x;
        auto last_diagonal = x - column_start;
        for (auto y = column_start; y <= s1len; y++) {
            auto old_diagonal  = column[y];
            auto possibilities = {column[y] + 1, column[y - 1] + 1, last_diagonal + (s1[y - 1] == s2[x - 1] ? 0 : 1)};
            column[y]          = std::min(possibilities);
            last_diagonal      = old_diagonal;
        }
    }
    auto result = column[s1len];
    delete[] column;
    return result;
}

} // namespace notf
