#pragma once

#include <assert.h>
#include <limits>
#include <stddef.h>

namespace notf {

/// @brief Specialized type for Index into arrays, vectors or other indexeable things.
/// This became necessary for the StackLayout class, where you could ask for an index of an Item which might not be in
/// the Layout at all.
/// In that case, the method should return an invalid index.
/// However, zero is a valid index and every other 'invalid' value would put the burden on the programmer using the
/// method to check against it.
///
/// The Index class forces you to call get() in order to verify the index.
/// In debug mode, getting an invalid index will raise an assert.
/// In release mode, this class will most likely be optimized away by the compiler.
class Index {

public: // typedefs
    /// @brief Index value type.
    using Type = size_t;

public: // methods
    /// @brief Default Constructor - produces an invalid Index.
    explicit Index()
        : m_value(BAD_INDEX)
    {
    }

    /// @brief Value Constructor.
    /// @param value    Index value.
    explicit Index(const Type value)
        : m_value(value)
    {
    }

    /// @brief Copy Constructor.
    /// @param other    Index to copy.
    Index(const Index& other)
        : m_value(other.m_value)
    {
    }

    // no assignment
    Index& operator=(const Index&) = delete;

    /// @brief Tests if this Index is valid.
    bool is_valid() const { return m_value != BAD_INDEX; }

    /// @brief Returns the Index value to be used.
    /// 'getting' an invalid Index will raise an assert.
    Type get() const
    {
        assert(is_valid());
        return m_value;
    }

    /// @brief Bool conversion to use an Index in an if-statement, for example.
    explicit operator bool() const { return is_valid(); }

public: // static methods
    /// @brief Produces an invalid Index.
    static Index make_invalid() { return Index(); }

private: // fields
    /// @brief Index value.
    const Type m_value;

private: // static fields
    // @brief The invalid Index value.
    static constexpr Type BAD_INDEX = std::numeric_limits<Type>::max();
};

} // namespace notf
