#include "app/path.hpp"

#include "fmt/format.h"

#include "common/assert.hpp"
#include "common/float.hpp"
#include "common/string.hpp"
#include "common/vector.hpp"

// ================================================================================================================== //

namespace { // anonymous

NOTF_USING_NAMESPACE;

std::string
construct_error_message(const char* input, const size_t error_position, const size_t error_length, const char* message)
{
    return fmt::format("Error when constructing path from string:\n"
                       "  \"{0}\"\n"  // original string
                       "  {1:>{2}}\n" // error indicator
                       "  {3}",       // error message
                       input, fmt::format("{0:^>{1}}", '^', error_length), error_position + error_length, message);
}

void check_concat(const Path& lhs, const Path& rhs)
{
    if (rhs.is_absolute()) {
        notf_throw_format(Path::construction_error,
                          "Cannot combine paths \"{}\" and \"{}\", because the latter one is absolute", lhs.to_string(),
                          rhs.to_string());
    }
    if (lhs.is_property() && rhs.size() > 0 && rhs[0] != "..") {
        notf_throw_format(Path::construction_error,
                          "Cannot combine paths \"{}\" and \"{}\", because the latter one must start with a \"..\"",
                          lhs.to_string(), rhs.to_string());
    }
}

} // namespace

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

Path::construction_error::~construction_error() = default;

Path::path_error::~path_error() = default;

Path::not_unique_error::~not_unique_error() = default;

// ================================================================================================================== //

Path::Path(std::vector<std::string>&& components, const bool is_absolute, const bool is_property)
    : m_components(std::move(components)), m_is_absolute(is_absolute), m_is_property(is_property)
{
    _normalize();
}

Path::Path(const std::string_view& string)
{
    if (string.empty()) {
        return;
    }

    // check if the path is absolute or not
    m_is_absolute = (string[0] == component_delimiter);

    const std::string::size_type property_delimiter_pos = string.find_first_of(property_delimiter);

    // additional delimiters after a property delimiter are not allowed
    if (property_delimiter_pos != std::string::npos) {
        const std::string::size_type extra_delimiter = string.find_first_of("/:", property_delimiter_pos + 1);
        if (extra_delimiter != std::string::npos) {
            notf_throw(construction_error,
                       construct_error_message(string.data(), extra_delimiter + 1, string.size() - extra_delimiter,
                                               "Additional delimiters after the property name are not allowed"));
        }

        // an empty property name is not allowed
        if (property_delimiter_pos == string.length() - 1) {
            notf_throw(construction_error, construct_error_message(string.data(), property_delimiter_pos + 1, 1,
                                                                   "An empty property name is not allowed"));
        }
    }

    { // parse the node components
        std::string::size_type string_pos{0};
        std::string::size_type delimiter_pos = string.find_first_of(component_delimiter, string_pos);
        while (delimiter_pos != std::string::npos) {
            if (auto component_length = delimiter_pos - string_pos) {
                m_components.emplace_back(&string[string_pos], component_length);
            }
            string_pos = delimiter_pos + 1;
            delimiter_pos = string.find_first_of(component_delimiter, string_pos);
        }

        // last node
        delimiter_pos = min(property_delimiter_pos, string.length());
        if (const auto component_length = delimiter_pos - string_pos) {
            m_components.emplace_back(&string[string_pos], component_length);
        }
    }

    // parse the optional property component
    if (property_delimiter_pos != std::string::npos) {
        std::string::size_type string_pos = property_delimiter_pos + 1;
        const auto component_length = string.length() - string_pos;
        NOTF_ASSERT(component_length > 0);
        m_components.emplace_back(&string[string_pos], component_length);
        m_is_property = true;
    }

    _normalize();
}

std::string Path::to_string() const
{
    if (m_components.empty()) {
        return "";
    }
    static const std::string delimiter(1, component_delimiter);
    if (is_node()) {
        return fmt::format("{}{}", is_absolute() ? delimiter : "", join(m_components, delimiter));
    }
    else {
        return fmt::format("{}{}:{}", is_absolute() ? delimiter : "",
                           join(m_components.cbegin(), --m_components.cend(), delimiter), m_components.back());
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
    if (m_components.size() != other.m_components.size()) {
        return false;
    }
    for (size_t i = 0; i < m_components.size(); ++i) {
        if (m_components[i] != other.m_components[i]) {
            return false;
        }
    }
    return true;
}

Path Path::operator+(const Path& other) const&
{
    check_concat(*this, other);

    std::vector<std::string> combined_components = m_components;
    extend(combined_components, other.m_components);
    return Path(std::move(combined_components), is_absolute(), other.is_property());
}

Path&& Path::operator+(Path&& other) &&
{
    check_concat(*this, other);

    extend(m_components, std::move(other.m_components));
    m_is_property = other.m_is_property;
    return std::move(*this);
}

void Path::_normalize()
{
    // normalize the component
    size_t next_valid_index = 0;
    for (size_t component_index = 0; component_index < m_components.size(); ++component_index) {
        const std::string& component = m_components[component_index];
        if (component == "." && m_components.size() > 1) {
            continue; // ignore "current directory" dot in all but the special "." (one dot only) path
        }

        if (component == "..") {
            if (next_valid_index == 0) {
                if (is_absolute()) {
                    notf_throw_format(construction_error, "Absolute path \"{}\" cannot be resolved", to_string());
                }
                // if the path is relative, we allow 0-n ".." components at the start
            }
            else {
                if (m_components[next_valid_index - 1] != "..") {
                    // if the component is a ".." and we know the parent node, go back one step
                    --next_valid_index;
                    continue;
                }
                // if the path is relative and the last component is already a ".." append the new component
            }
        }

        // store the component
        if (next_valid_index != component_index) {
            std::swap(m_components[next_valid_index], m_components[component_index]);
        }
        ++next_valid_index;
    }

    NOTF_ASSERT(next_valid_index <= m_components.size());
    m_components.resize(next_valid_index);
    m_components.shrink_to_fit();
}

NOTF_CLOSE_NAMESPACE