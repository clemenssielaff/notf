#pragma once

#include <iostream>
#include <type_traits>

namespace notf {

//====================================================================================================================//

/// Strongly typed integral identifier.
/// Is useful if you have multiple types of identifiers that are all the same underlying type and don't want them to be
/// assigneable / comparable.
template<typename type, typename underlying_type, typename... aux>
struct IdType {

    /// Type identified by the ID.
    using type_t = type;

    /// Integer type used for ID arithmetic.
    using underlying_t = underlying_type;

    /// Auxiliary types.
    /// Useful, if you want to attach more type information to the ID type.
    using aux_ts = std::tuple<aux...>;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Invalid Id.
    static constexpr underlying_type INVALID = 0;

    /// Identifier value of this instance.
    underlying_type value;

    // methods -------------------------------------------------------------------------------------------------------//
    /// No default constructor.
    IdType() = delete;

    /// Value constructor.
    IdType(const underlying_type id) : value(id) {}

    /// Copy constructor from typed id.
    template<typename... Ts>
    IdType(const IdType<type_t, underlying_t, Ts...> id) : value(id.value)
    {}

    /// Explicit invalid Id generator.
    static IdType invalid() { return IdType(INVALID); }

    /// Equality operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator==(const OTHER& rhs) const
    {
        return value == rhs.id;
    }
    bool operator==(const underlying_t& rhs) const { return value == rhs; }

    /// Inequality operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator!=(const OTHER& rhs) const
    {
        return value != rhs.id;
    }
    bool operator!=(const underlying_t& rhs) const { return value != rhs; }

    /// Lesser-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator<(const OTHER& rhs) const
    {
        return value < rhs.id;
    }
    bool operator<(const underlying_t& rhs) const { return value < rhs; }

    /// Lesser-or-equal-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator<=(const OTHER& rhs) const
    {
        return value <= rhs.id;
    }
    bool operator<=(const underlying_t& rhs) const { return value <= rhs; }

    /// Greater-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator>(const OTHER& rhs) const
    {
        return value > rhs.id;
    }
    bool operator>(const underlying_t& rhs) const { return value > rhs; }

    /// Greater-or-equal-than operator.
    /// @param rhs  Value to test against.
    template<typename OTHER, typename = std::enable_if_t<std::is_same<typename OTHER::type_t, type_t>::value>>
    bool operator>=(const OTHER& rhs) const
    {
        return value >= rhs.id;
    }
    bool operator>=(const underlying_t& rhs) const { return value >= rhs; }

    /// Checks if this Id is valid or not.
    bool is_valid() const { return value != INVALID; }
    explicit operator bool() const { return is_valid(); }

    /// Cast back to the underlying type.
    /// Must be explicit to avoid comparison between different Id types.
    explicit operator underlying_type() const { return value; }

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// We cannot use SFINAE in the struct template because of the variadic template parameters, but we can use it here:
    using fails_if_underlying_type_is_not_integral =
        typename std::enable_if_t<std::is_integral<underlying_type>::value>;
};

//====================================================================================================================//

/// Prints the contents of an Id into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param id   Id to print.
/// @return Output stream for further output.
template<typename Type, typename underlying_type>
std::ostream& operator<<(std::ostream& out, const IdType<Type, underlying_type>& id)
{
    return out << id.value;
}

} // namespace notf
