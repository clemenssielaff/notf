#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/string_view.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Path;
} // namespace access

// ================================================================================================================== //

/// Every Path is immutable, and guaranteed valid when created successfully.
class Path {

    friend class access::_Path<Node>;

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// Path("A") can refer to either a Node or a Property.
    /// Path(":A") is definitely a Property, Path(".A") is definitely a node.
    enum Kind : char {
        NODE,      //< Definitely a node.
        AMBIGUOUS, //< Single component Path without enforced node or property prefix
        PROPERTY,  //< Definitely a property.
    };

public:
    /// Access types.
    template<class T>
    using Access = access::_Path<T>;

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
    Path(std::vector<std::string>&& components, bool is_absolute, Kind kind);

public:
    /// Default constructor.
    Path() = default;

    /// @{
    /// Value constructor.
    /// @param string       Input to parse as a path.
    /// @throws construction_error  If the string failed to be parsed.
    explicit Path(notf::string_view to_string);
    explicit Path(const std::string& string) : Path(notf::string_view(string)) {}
    explicit Path(const char* string) : Path(notf::string_view(string)) {}
    /// @}

    /// Move constructor.
    /// @param other        Path to move from.
    /// @throws construction_error  If the string failed to be parsed.
    Path(Path&& other) noexcept
    {
        m_components = std::move(other.m_components);
        m_is_absolute = other.m_is_absolute;
        m_kind = other.m_kind;
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
    bool is_node() const { return m_kind != PROPERTY; }

    /// Checks whether or not the last component in the Path is a property name.
    bool is_property() const { return m_kind != NODE; }

    /// Returns the normalized string representation of this Path.
    std::string to_string() const;

    /// Number of components in the path.
    size_t size() const { return m_components.size(); }

    /// Checks whether this Path shares the first n components with the other.
    /// @param other    Other path with n components.
    bool begins_with(const Path& other) const;

    /// Returns the nth component name of the path.
    /// @throws path_error  If the index does not identify a component of this path.
    const std::string& operator[](const size_t index) const
    {
        if (index < size()) {
            return m_components[index];
        }
        notf_throw(path_error, "Index {} is out of bounds for path \"{}\"", index, to_string());
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

    /// If this Path identifies a Node, a Property or is ambiguous.
    Kind m_kind = AMBIGUOUS;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Path<Node> {
    friend class notf::Node;

    /// Creates a new Path.
    /// @param components   Components for an absolute node path.
    static Path create(std::vector<std::string>&& components)
    {
        return Path(std::move(components), /* is_absolute = */ true, Path::Kind::NODE);
    }
};

// ================================================================================================================== //

namespace literals {

/// String literal for convenient Path construction.
inline Path operator"" _path(const char* input, size_t size) { return Path(notf::string_view(input, size)); }

} // namespace literals

NOTF_CLOSE_NAMESPACE
