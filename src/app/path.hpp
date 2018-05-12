#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/string_view.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Every Path is immutable, and guaranteed valid when created successfully.
class Path {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Thrown when a Path could not be constructed.
    NOTF_EXCEPTION_TYPE(no_path);

    /// Thrown when an invalid index was requested.
    NOTF_EXCEPTION_TYPE(token_error);

    /// Delimiter character used to separate nodes in the path.
    static constexpr char node_delimiter = '/';

    /// Delimiter character used to denote a final property token in the path.
    static constexpr char property_delimiter = ':';

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Value constructor.
    /// For internal use only.
    Path(std::vector<std::string>&& tokens, const bool is_absolute, const bool is_property);

public:
    /// Default constructor.
    Path() = default;

    /// @{
    /// Value constructor.
    /// @param string       Input to parse as a path.
    /// @throws no_path     If the string failed to be parsed.
    Path(const std::string_view& string);
    Path(const std::string& string) : Path(std::string_view(string)) {}
    Path(const char* string) : Path(std::string_view(string)) {}
    /// @}

    /// Move constructor.
    /// @param other        Path to move from.
    /// @throws no_path     If the string failed to be parsed.
    Path(Path&& other)
    {
        m_tokens = std::move(other.m_tokens);
        m_is_absolute = other.m_is_absolute;
        m_is_property = other.m_is_property;
        _normalize();
    }

    /// Checks whether the Path is empty.
    bool is_empty() const { return m_tokens.empty(); }

    /// Tests if this Path is absolute or not.
    /// Absolute paths begin with a forward slash.
    bool is_absolute() const { return m_is_absolute; }

    /// Tests if this Path is relative or not.
    bool is_relative() const { return !m_is_absolute; }

    /// Checks whether or not the last token in the Path is a node name.
    bool is_node() const { return !m_is_property; }

    /// Checks whether or not the last token in the Path is a property name.
    bool is_property() const { return m_is_property; }

    /// Returns the normalized string representation of this Path.
    std::string string() const;

    /// Number of nodes in the path.
    size_t node_count() const { return m_tokens.size() - (m_is_property ? 1 : 0); }

    /// Returns the nth node name of the path.
    /// @throws token_error If the index does not identify a node of this path.
    const std::string& node(size_t index) const
    {
        if (index < node_count()) {
            return m_tokens[index];
        }
        notf_throw_format(token_error, "Index {} is out of bounds for path \"{}\"", index, string());
    }

    /// Returns the name of the property identified by this path.
    /// @throws token_error If this path does not contain a property.
    const std::string& property() const
    {
        if (m_is_property) {
            return m_tokens.back();
        }
        notf_throw_format(token_error, "Path \"{}\" does not contain a property", string());
    }

    /// Lexical equality comparison of two Paths.
    /// @param other    Path to compare against.
    bool operator==(const Path& other) const;

    /// Lexical inequality comparison of two Paths.
    /// @param other    Path to compare against.
    bool operator!=(const Path& other) const { return !(*this == other); }

    /// Concatenates this and another relative path.
    /// @param other    Relative path to append to this one.
    /// @returns        The normalized concatenation of this and other.
    /// @throws no_path If the other path is absolute.
    Path operator+(const Path& other) const&;

    /// Effective concatenation of this and another relative path if this is an rvalue.
    /// Does not perform normalization.
    /// @param other    Relative path to append to this one.
    /// @returns        The unnormalized concatenation of this and other.
    /// @throws no_path If the other path is absolute.
    Path&& operator+(Path&& other) &&;

private:
    /// Normalizes the tokens in this path.
    void _normalize();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All Path tokens.
    std::vector<std::string> m_tokens;

    /// Whether or not the Path is absolute or relative.
    bool m_is_absolute = false;

    /// Whether or not the last token in the Path is a property name.
    bool m_is_property = false;
};

NOTF_CLOSE_NAMESPACE
