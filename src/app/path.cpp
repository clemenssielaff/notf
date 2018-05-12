#include "app/path.hpp"

#include "fmt/format.h"

#include "common/assert.hpp"
#include "common/float.hpp"
#include "common/string.hpp"
#include "common/vector.hpp"

// ===================================================================================================================//

namespace { // anonymous

std::string
construct_error_message(const char* input, const size_t error_position, const size_t error_length, const char* message)
{
    return fmt::format("Error when constructing path from string:\n"
                       "  \"{0}\"\n"  // original string
                       "  {1:>{2}}\n" // error indicator
                       "  {3}",       // error message
                       input, fmt::format("{0:^>{1}}", '^', error_length), error_position + error_length, message);
}

} // namespace

// ===================================================================================================================//

NOTF_OPEN_NAMESPACE

Path::no_path::~no_path() = default;

Path::token_error::~token_error() = default;

// ===================================================================================================================//

Path::Path(std::vector<std::string>&& tokens, const bool is_absolute, const bool is_property)
    : m_is_absolute(is_absolute), m_is_property(is_property)
{
    _store_normalized_tokens(std::move(tokens));
}

Path::Path(const std::string_view& string)
{
    if (string.empty()) {
        return;
    }

    // check if the path is absolute or not
    m_is_absolute = (string[0] == node_delimiter);

    const std::string::size_type property_delimiter_pos = string.find_first_of(property_delimiter);

    // additional delimiters after a property delimiter are not allowed
    if (property_delimiter_pos != std::string::npos) {
        const std::string::size_type extra_delimiter = string.find_first_of("/:", property_delimiter_pos + 1);
        if (extra_delimiter != std::string::npos) {
            notf_throw(no_path,
                       construct_error_message(string.data(), extra_delimiter + 1, string.size() - extra_delimiter,
                                               "Additional delimiters after the property name are not allowed"));
        }

        // an empty property name is not allowed
        if (property_delimiter_pos == string.length() - 1) {
            notf_throw(no_path, construct_error_message(string.data(), property_delimiter_pos + 1, 1,
                                                        "An empty property name is not allowed"));
        }
    }

    std::vector<std::string> tokens;
    { // parse the node tokens
        std::string::size_type string_pos{0};
        std::string::size_type delimiter_pos = string.find_first_of(node_delimiter, string_pos);
        while (delimiter_pos != std::string::npos) {
            if (auto token_length = delimiter_pos - string_pos) {
                tokens.emplace_back(&string[string_pos], token_length);
            }
            string_pos = delimiter_pos + 1;
            delimiter_pos = string.find_first_of(node_delimiter, string_pos);
        }

        // last node
        delimiter_pos = min(property_delimiter_pos, string.length());
        if (const auto token_length = delimiter_pos - string_pos) {
            tokens.emplace_back(&string[string_pos], token_length);
        }
    }

    // parse the optional property token
    if (property_delimiter_pos != std::string::npos) {
        std::string::size_type string_pos = property_delimiter_pos + 1;
        const auto token_length = string.length() - string_pos;
        NOTF_ASSERT(token_length > 0);
        tokens.emplace_back(&string[string_pos], token_length);
        m_is_property = true;
    }

    _store_normalized_tokens(std::move(tokens));
}

Path::Path(Path&& other)
{
    m_is_absolute = other.m_is_absolute;
    m_is_property = other.m_is_property;

    if (other.m_is_normalized) {
        m_tokens = std::move(other.m_tokens);
    }
    else {
        _store_normalized_tokens(std::move(other.m_tokens));
    }
    m_is_normalized = true;
}

std::string Path::string() const
{
    if (m_tokens.empty()) {
        return "";
    }
    static const std::string delimiter(1, node_delimiter);
    if (is_node()) {
        return fmt::format("{}{}", is_absolute() ? delimiter : "", join(m_tokens, delimiter));
    }
    else {
        return fmt::format("{}{}:{}", is_absolute() ? delimiter : "",
                           join(m_tokens.cbegin(), --m_tokens.cend(), delimiter), m_tokens.back());
    }
}

bool Path::operator==(const Path& other) const
{
    if (m_is_property != other.m_is_property) {
        return false;
    }
    if (m_is_absolute != other.m_is_absolute) {
        return false;
    }
    if (m_tokens.size() != other.m_tokens.size()) {
        return false;
    }
    for (size_t i = 0; i < m_tokens.size(); ++i) {
        if (m_tokens[i] != other.m_tokens[i]) {
            return false;
        }
    }
    return true;
}

Path Path::operator+(const Path& other) const&
{
    if (other.is_absolute()) {
        notf_throw_format(no_path, "Cannot combine paths \"{}\" and \"{}\", because the latter one is absolute",
                          string(), other.string());
    }

    std::vector<std::string> combined_tokens = m_tokens;
    extend(combined_tokens, other.m_tokens);
    return Path(std::move(combined_tokens), is_absolute(), other.is_property());
}
Path&& Path::operator+(Path&& other) &&
{
    if (other.is_absolute()) {
        notf_throw_format(no_path, "Cannot combine paths \"{}\" and \"{}\", because the latter one is absolute",
                          string(), other.string());
    }

    extend(m_tokens, std::move(other.m_tokens));
    m_is_property = other.m_is_property;
    m_is_normalized = false;
    return std::move(*this);
}

void Path::_store_normalized_tokens(const std::vector<std::string>&& tokens)
{
    m_tokens.reserve(tokens.size());

    // normalize the token
    for (const std::string& token : tokens) {
        if (token == "." && tokens.size() > 1) {
            continue; // ignore "current directory" dot in all but the special "." (one dot only) path
        }

        if (token == "..") {
            if (m_tokens.empty()) {
                if (is_absolute()) {
                    m_tokens = std::move(tokens); // temporarly create an invalid path
                    notf_throw_format(no_path, "Absolute path \"{}\" cannot be resolved", string());
                }
                // if the path is relative and empty, we store the token
            }
            else {
                if (m_tokens.back() != "..") {
                    // if the token is a ".." and we know the parent node, go back one step
                    m_tokens.pop_back();
                    continue;
                }
                // if the path is relative and the last token is already a ".." append the new token
            }
        }

        // store the token
        m_tokens.emplace_back(token);
    }
    m_tokens.shrink_to_fit();
    m_is_normalized = true;
}

NOTF_CLOSE_NAMESPACE
