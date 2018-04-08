#pragma once

#include <functional>
#include <ostream>
#include <type_traits>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Strongly typed integral identifier.
/// Is useful if you have multiple types of identifiers that are all the same underlying type and don't want them to be
/// assigneable / comparable.
template<typename type, typename underlying_type, typename... aux>
struct IdType {

    static_assert(std::is_integral<underlying_type>::value, "The underlying type of an IdType must be integral");

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Type identified by the ID.
    using type_t = type;

    /// Integer type used for ID arithmetic.
    using underlying_t = underlying_type;

    /// Auxiliary types.
    /// Useful, if you want to attach more type information to the ID type.
    using aux_ts = std::tuple<aux...>;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Identifier value of this ID.
    underlying_type m_value;

public:
    /// Invalid Id.
    static constexpr underlying_type INVALID = 0;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// No default constructor.
    IdType() = delete;

    /// Value constructor.
    IdType(const underlying_type value) : m_value(value) {}

    /// Copy constructor from typed ID.
    template<typename... Ts>
    IdType(const IdType<type_t, underlying_t, Ts...>& id) : m_value(id.value())
    {}

    /// Explicit invalid Id generator.
    static IdType invalid() { return IdType(INVALID); }

    /// Identifier value of this ID.
    const underlying_t& value() const { return m_value; }

    /// @{
    /// Equality operator.
    /// @param rhs  ID to test against.
    bool operator==(const IdType& rhs) const { return m_value == rhs.m_value; }
    bool operator==(const underlying_t& rhs) const { return m_value == rhs; }
    /// @}

    /// @{
    /// Inequality operator.
    /// @param rhs  ID to test against.
    bool operator!=(const IdType& rhs) const { return m_value != rhs.m_value; }
    bool operator!=(const underlying_t& rhs) const { return m_value != rhs; }
    /// @}

    /// @{
    /// Lesser-than operator.
    /// @param rhs  Value to test against.
    bool operator<(const IdType& rhs) const { return m_value < rhs.m_value; }
    bool operator<(const underlying_t& rhs) const { return m_value < rhs; }
    /// @}

    /// @{
    /// Lesser-or-equal-than operator.
    /// @param rhs  Value to test against.
    bool operator<=(const IdType& rhs) const { return m_value <= rhs.m_value; }
    bool operator<=(const underlying_t& rhs) const { return m_value <= rhs; }
    /// @}

    /// @{
    /// Greater-than operator.
    /// @param rhs  Value to test against.
    bool operator>(const IdType& rhs) const { return m_value > rhs.m_value; }
    bool operator>(const underlying_t& rhs) const { return m_value > rhs; }
    /// @}

    /// @{
    /// Greater-or-equal-than operator.
    /// @param rhs  Value to test against.
    bool operator>=(const IdType& rhs) const { return m_value >= rhs.m_value; }
    bool operator>=(const underlying_t& rhs) const { return m_value >= rhs; }
    /// @}

    /// Checks if this Id is valid or not.
    bool is_valid() const { return m_value != INVALID; }

    /// Checks if this Id is valid or not.
    explicit operator bool() const { return is_valid(); }

    /// Cast back to the underlying type.
    /// Must be explicit to avoid comparison between different Id types.
    explicit operator underlying_type() const { return m_value; }
};

//====================================================================================================================//

/// Prints the contents of an Id into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param id   Id to print.
/// @return Output stream for further output.
template<typename Type, typename underlying_type, typename... aux>
std::ostream& operator<<(std::ostream& out, const IdType<Type, underlying_type, aux...>& id)
{
    return out << id.value();
}

NOTF_CLOSE_NAMESPACE

//====================================================================================================================//

namespace std {

/// std::hash specialization for IdTypes.
template<typename type, typename underlying_type, typename... aux>
struct hash<notf::IdType<type, underlying_type, aux...>> {
    size_t operator()(const notf::IdType<type, underlying_type, aux...>& id) const
    {
        return static_cast<size_t>(id.value());
    }
};

} // namespace std
