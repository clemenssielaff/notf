#pragma once

#include <iostream>
#include <type_traits>

namespace notf {

//====================================================================================================================//

/// Strongly typed integral identifier.
/// Is useful if you have multiple types of identifiers that are all the same underlying type and don't want them to be
/// assigneable / comparable.
template<typename Type, typename underlying_type, typename = std::enable_if_t<std::is_integral<underlying_type>::value>>
struct IdType {

    /// Type identified by the ID.
    using type_t = Type;

    /// Integer type used for ID arithmetic.
    using underlying_t = underlying_type;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Identifier value of this instance.
    const underlying_type id;

    /// Invalid Id.
    static constexpr underlying_type INVALID = 0;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor, produces an invalid ID.
    IdType() : id(INVALID) {}

    /// Value constructor.
    IdType(const underlying_type id) : id(id) {}

    /// Explicit invalid Id generator.
    static IdType invalid() { return {}; }

    /// Equality operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator==(const OTHER& rhs) const
    {
        return id == rhs.id;
    }
    bool operator==(const underlying_t& rhs) const { return id == rhs; }

    /// Inequality operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator!=(const OTHER& rhs) const
    {
        return id != rhs.id;
    }
    bool operator!=(const underlying_t& rhs) const { return id != rhs; }

    /// Lesser-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator<(const OTHER& rhs) const
    {
        return id < rhs.id;
    }
    bool operator<(const underlying_t& rhs) const { return id < rhs; }

    /// Lesser-or-equal-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator<=(const OTHER& rhs) const
    {
        return id <= rhs.id;
    }
    bool operator<=(const underlying_t& rhs) const { return id <= rhs; }

    /// Greater-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator>(const OTHER& rhs) const
    {
        return id > rhs.id;
    }
    bool operator>(const underlying_t& rhs) const { return id > rhs; }

    /// Greater-or-equal-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator>=(const OTHER& rhs) const
    {
        return id >= rhs.id;
    }
    bool operator>=(const underlying_t& rhs) const { return id >= rhs; }

    /// Checks if this Id is valid or not.
    bool is_valid() const { return id != INVALID; }
    explicit operator bool() const { return is_valid(); }

    /// Cast back to the underlying type.
    /// Must be explicit to avoid comparison between different Id types.
    explicit operator underlying_type() const { return id; }
};

//====================================================================================================================//

/// Prints the contents of an Id into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param id   Id to print.
/// @return Output stream for further output.
template<typename Type, typename underlying_type>
std::ostream& operator<<(std::ostream& out, const IdType<Type, underlying_type>& id)
{
    return out << id.id;
}

} // namespace notf
