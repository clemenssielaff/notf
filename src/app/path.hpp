#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/string_view.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Every Path is immutable, and guaranteed valid when created successfully.
class Path {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Thrown when a Path could not be constructed.
    NOTF_EXCEPTION_TYPE(construction_error);

    /// Thrown when an invalid path component was requested.
    NOTF_EXCEPTION_TYPE(path_error);

    /// Thrown when a name or path is not unique.
    NOTF_EXCEPTION_TYPE(not_unique_error);

    /// Delimiter character used to separate components in the path.
    static constexpr char component_delimiter = '/';

    /// Delimiter character used to denote a final property component in the path.
    static constexpr char property_delimiter = ':';

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Value constructor.
    /// For internal use only.
    Path(std::vector<std::string>&& components, const bool is_absolute, const bool is_property);

public:
    /// Default constructor.
    Path() = default;

    /// @{
    /// Value constructor.
    /// @param string       Input to parse as a path.
    /// @throws construction_error  If the string failed to be parsed.
    Path(const std::string_view& to_string);
    Path(const std::string& string) : Path(std::string_view(string)) {}
    Path(const char* string) : Path(std::string_view(string)) {}
    /// @}

    /// Move constructor.
    /// @param other        Path to move from.
    /// @throws construction_error  If the string failed to be parsed.
    Path(Path&& other)
    {
        m_components = std::move(other.m_components);
        m_is_absolute = other.m_is_absolute;
        m_is_property = other.m_is_property;
        _normalize();
    }

    /// Checks whether the Path is empty.
    bool is_empty() const { return m_components.empty(); }

    /// Tests if this Path is absolute or not.
    /// Absolute paths begin with a forward slash.
    bool is_absolute() const { return m_is_absolute; }

    /// Tests if this Path is relative or not.
    bool is_relative() const { return !m_is_absolute; }

    /// Checks whether or not the last component in the Path is a node name.
    bool is_node() const { return !m_is_property; }

    /// Checks whether or not the last component in the Path is a property name.
    bool is_property() const { return m_is_property; }

    /// Returns the normalized string representation of this Path.
    std::string to_string() const;

    /// Number of components in the path.
    size_t size() const { return m_components.size(); }

    /// Returns the nth component name of the path.
    /// @throws path_error  If the index does not identify a component of this path.
    const std::string& operator[](const size_t index) const
    {
        if (index < size()) {
            return m_components[index];
        }
        notf_throw_format(path_error, "Index {} is out of bounds for path \"{}\"", index, to_string());
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
    /// @throws construction_error  If the other path is absolute.
    Path operator+(const Path& other) const&;

    /// Effective concatenation of this and another relative path if this is an rvalue.
    /// Does not perform normalization.
    /// @param other    Relative path to append to this one.
    /// @returns        The unnormalized concatenation of this and other.
    /// @throws construction_error  If the other path is absolute.
    Path&& operator+(Path&& other) &&;

private:
    /// Normalizes the components in this path.
    void _normalize();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All Path components.
    std::vector<std::string> m_components;

    /// Whether or not the Path is absolute or relative.
    bool m_is_absolute = false;

    /// Whether or not the last component in the Path is a property name.
    bool m_is_property = false;
};

NOTF_CLOSE_NAMESPACE
