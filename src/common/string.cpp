#include "notf/common/string.hpp"

#include <algorithm>
#include <numeric>

#include "notf/meta/assert.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

std::vector<std::string> split(const std::string& input, const char delimiter) {
    std::vector<std::string> result;
    if (input.empty()) { return result; }
    std::string::size_type start_pos{0};
    std::string::size_type end_pos = input.find_first_of(delimiter, start_pos);
    while (end_pos != std::string::npos) {
        if (auto len = end_pos - start_pos) { result.emplace_back(input.substr(start_pos, len)); }
        start_pos = end_pos + 1;
        end_pos = input.find_first_of(delimiter, start_pos);
    }
    std::string last_entry = input.substr(start_pos, end_pos - start_pos);
    if (!last_entry.empty()) { result.emplace_back(std::move(last_entry)); }
    return result;
}

bool istarts_with(const std::string& input, const std::string& prefix) {
    // while it may look unnecesarily expensive, this is in fact the fastest way I found (so far)
    if (prefix.size() > input.size()) { return false; }

    std::string relevant_input = input.substr(0, prefix.length());
    std::transform(relevant_input.begin(), relevant_input.end(), relevant_input.begin(),
                   [](auto c) { return static_cast<std::string::value_type>(::tolower(c)); });

    std::string prefix_lower = prefix;
    std::transform(prefix_lower.begin(), prefix_lower.end(), prefix_lower.begin(),
                   [](auto c) { return static_cast<std::string::value_type>(::tolower(c)); });

    return (relevant_input.compare(0, prefix.length(), prefix_lower) == 0);
}

bool ends_with(const std::string& input, const std::string& postfix) {
    const auto input_size = input.size();
    const auto postfix_size = postfix.size();
    if (postfix_size > input_size) { return false; }
    return (input.compare(input_size - postfix_size, postfix_size, postfix) == 0);
}

bool iends_with(const std::string& input, const std::string& postfix) {
    const auto input_size = input.size();
    const auto postfix_size = postfix.size();
    if (postfix_size > input_size) { return false; }
    std::string relevant_input = input.substr(input_size - postfix_size, input_size);
    std::transform(relevant_input.begin(), relevant_input.end(), relevant_input.begin(),
                   [](auto c) { return static_cast<std::string::value_type>(::tolower(c)); });
    std::string postfix_lower = postfix;
    std::transform(postfix_lower.begin(), postfix_lower.end(), postfix_lower.begin(),
                   [](auto c) { return static_cast<std::string::value_type>(::tolower(c)); });
    return (relevant_input.compare(0, postfix_size, postfix_lower) == 0);
}

bool icompare(const std::string& left, const std::string& right) {
    if (left.length() != right.length()) { return false; }
    return std::equal(right.begin(), right.end(), left.begin(),
                      [](auto a, auto b) -> bool { return ::tolower(a) == ::tolower(b); });
}

std::string join(const std::vector<std::string>::const_iterator begin,
                 const std::vector<std::string>::const_iterator end, const std::string& delimiter) {
    if (begin == end) { return ""; }
    const auto distance = std::distance(begin, end);
    if (distance == 1) { return *begin; }
    NOTF_ASSERT(distance > 0);

    std::string result;
    { // reserve enough space in the result
        size_t size = static_cast<size_t>(distance - 1) * delimiter.size();
        for (auto it = begin; it != end; ++it) {
            size += it->size();
        }
        if (size == 0) { return result; }
        result.reserve(size);
    }

    // join the strings
    if (delimiter.empty()) {
        for (auto it = begin; it != end; ++it) {
            result.append(*it);
        }
    } else {
        for (auto it = begin; it != end - 1; ++it) {
            result.append(*it);
            result.append(delimiter);
        }
        result.append(*(end - 1));
    }

    return result;
}

NOTF_CLOSE_NAMESPACE
