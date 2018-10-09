#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../meta/assert.hpp"
#include "../meta/numeric.hpp"
#include "./common.hpp"
#include "./tuple.hpp"
#include "./variant.hpp"

NOTF_OPEN_NAMESPACE

// msgpack ========================================================================================================== //

class MsgPack {

    friend struct Accessor<MsgPack, MsgPack>;
#ifdef NOTF_TEST
    friend struct Accessor<MsgPack, Tester>;
#endif

    // TODO: maybe DO store shared ptrs to other MsgPacks in order to copy them faster

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Data types.
    using None = None;
    using Bool = bool;
    using Int = int64_t;
    using Uint = uint64_t;
    using Real = double;
    using String = std::string;
    using Binary = std::vector<char>;
    using Array = std::vector<MsgPack>;
    using Map = std::map<MsgPack, MsgPack>;
    using Extension = std::pair<uint8_t, Binary>;

    /// Data type identifier.
    enum class Type {
        NONE,
        BOOL,
        INT,
        UINT,
        REAL,
        STRING,
        BINARY,
        ARRAY,
        MAP,
        EXTENSION,
    };

private:
    /// All types returnable by value.
    using value_types = std::tuple<None, Bool, Int, Uint, Real>;

    /// All types returnable by reference (every type that is not returned by value).
    using container_types = std::tuple<String, Binary, Array, Map, Extension>;

    /// Variant type containing all value types.
    using Variant = tuple_to_variant_t<concat_tuple_t<value_types, container_types>>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default constructor, constructs a "None" MsgPack.
    MsgPack() = default;
    MsgPack(None) : MsgPack() {}

    /// Boolean constructors.
    /// @param value    Value of the MsgPack.
    MsgPack(bool value) : m_value(value) {}
    MsgPack(void*) = delete; /// avoid implicit cast of pointer to bool

    /// Signed integer constructors.
    /// @param value    Value of the MsgPack.
    MsgPack(Int value) : m_value(value) {}
    MsgPack(int8_t value) : MsgPack(Int(value)) {}
    MsgPack(int16_t value) : MsgPack(Int(value)) {}
    MsgPack(int32_t value) : MsgPack(Int(value)) {}

    /// Unsigned integer constructors.
    /// @param value    Value of the MsgPack.
    MsgPack(Uint value) : m_value(value) {}
    MsgPack(uint8_t value) : MsgPack(Uint(value)) {}
    MsgPack(uint16_t value) : MsgPack(Uint(value)) {}
    MsgPack(uint32_t value) : MsgPack(Uint(value)) {}

    /// Real constructors.
    /// @param value    Value of the MsgPack.
    MsgPack(Real value) : m_value(value) {}
    MsgPack(float value) : MsgPack(Real(value)) {}

    /// Explicit constructor for Strings to avoid them being cast to an Array.
    /// @param value    Value of the MsgPack.
    MsgPack(String value) : m_value(String(std::move(value))) {}
    MsgPack(const char* value) : m_value(String(value)) {}

    /// Constructor for Binary objects.
    /// @param value    Value of the MsgPack.
    MsgPack(const Binary& value) : m_value(Binary(std::begin(value), std::end(value))) {}

    /// Constructor for Array-like objects (vector, set etc).
    /// @param value    Array-like object to initialize a MsgPack Array with.
    template<class T, class value_t = typename T::value_type,
             class = std::enable_if_t<std::is_constructible_v<MsgPack, value_t>                  // value is compatible
                                      && !std::is_same_v<typename Binary::value_type, value_t>>> // but not binary
    MsgPack(const T& value) : m_value(Array(value.begin(), value.end()))
    {}

    /// Constructor for Map-like objects (map, unordered_map etc).
    /// @param value    Map-like object to initialize a MsgPack Map with.
    template<class T, class key_t = typename T::key_type, class value_t = typename T::mapped_type,
             class = std::enable_if_t<std::is_constructible_v<MsgPack, key_t>        // key is compatible
                                      && std::is_constructible_v<MsgPack, value_t>>> // value is compatible
    MsgPack(const T& value) : m_value(Map(std::begin(value), std::end(value)))
    {}

    /// The data type currently held by this MsgPack.
    Type get_type() const noexcept { return static_cast<Type>(m_value.index()); }

    /// Value Getter.
    /// Has two overloads, one for types returned by value; one for types returned by const reference.
    /// @param success  Is set to true, iff a non-empty value was returned.
    /// @returns        A new T by value.
    template<class T>
    std::enable_if_t<is_one_of_variant<T, Variant>(), std::conditional_t<is_one_of_tuple_v<T, value_types>, T, const T&>>
    get(bool& success) const noexcept
    {
        // return the value of the requested type
        if (std::holds_alternative<T>(m_value)) {
            success = true;
            return std::get<T>(m_value);
        }

        // integers can be requested with the wrong type, if the value is safely castable
        if constexpr (std::is_same_v<T, Int>) {
            if (std::holds_alternative<Uint>(m_value)) {
                if (auto value = std::get<Uint>(m_value); value < max_value<Int>()) {
                    return static_cast<T>(value);
                }
            }
        }
        else if constexpr (std::is_same_v<T, Uint>) {
            if (std::holds_alternative<Int>(m_value)) {
                if (auto value = std::get<Int>(m_value); value >= 0) {
                    return static_cast<T>(value);
                }
            }
        }

        // even the biggest unsigned integer can be cast to double
        else if constexpr (std::is_same_v<T, Real>) {
            if (std::holds_alternative<Int>(m_value)) {
                return static_cast<T>(std::get<Int>(m_value));
            }
            else if (std::holds_alternative<Uint>(m_value)) {
                return static_cast<T>(std::get<Uint>(m_value));
            }
        }

        // return empty value
        success = false;
        static_assert(std::is_nothrow_constructible_v<T>);
        static const T empty = {};
        return empty;
    }

    /// Get the value type or a default-constructed value.
    /// @returns Value or const-ref, depending on T.
    template<class T>
    auto get() const noexcept
    {
        bool ignored;
        return get<T>(ignored);
    }

    /// If this MsgPack contains an array, returns the `i`th element of that array.
    /// This is a convenience function, if you plan to make extensive use of the map, consider `get`ting the underlying
    /// MsgPack::Array object directly.
    /// @param index            Index of the requested element in the array.
    /// @throws value_error     If the MsgPack does not contain an Array.
    /// @throws out_of_bounds   If the index is larger than the largest index in the Array.
    const MsgPack& operator[](const size_t index) const
    {
        if (!std::holds_alternative<Array>(m_value)) {
            NOTF_THROW(value_error, "MsgPack object is not an Array");
        }

        const auto& array = std::get<Array>(m_value);
        if (index < array.size()) {
            return array[index];
        }

        NOTF_THROW(out_of_bounds, "MsgPack Array has only {} elements, requested index was {}", array.size(), index);
    }

    /// If this MsgPack contains a map, returns the element matching the given string key.
    /// This is a convenience function, if you plan to make extensive use of the map, consider `get`ting the underlying
    /// MsgPack::Map object directly.
    /// @param key              String key of the requested element in the map.
    /// @throws value_error     If the MsgPack does not contain an Array.
    /// @throws out_of_bounds   If the index is larger than the largest index in the Array.
    const MsgPack& operator[](const std::string& key) const
    {
        if (!std::holds_alternative<Map>(m_value)) {
            NOTF_THROW(value_error, "MsgPack object is not a Map");
        }

        const auto& map = std::get<Map>(m_value);
        if (auto it = map.find(key); it != map.end()) {
            return it->second;
        }

        NOTF_THROW(out_of_bounds, "MsgPack Map does not contain requested key \"{}\"", key);
    }

    /// Comparison operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator==(const MsgPack& rhs) const { return m_value == rhs.m_value; }

    /// Less-than operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator<(const MsgPack& rhs) const { return m_value < rhs.m_value; }

    /// Inequality operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator!=(const MsgPack& rhs) const { return !(*this == rhs); }

    /// Less-or-equal-than operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator<=(const MsgPack& rhs) const { return !(rhs < *this); }

    /// Larger-than operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator>(const MsgPack& rhs) const { return (rhs < *this); }

    /// Larger-or-equal-than operator.
    /// @param rhs  Other MsgPack object to compare against.
    bool operator>=(const MsgPack& rhs) const { return !(*this < rhs); }

    /// Dump the MsgPack into a data stream.
    /// @param os   Output data stream to serialize into.
    void serialize(std::ostream& os) const { _serialize(/* depth= */ 0, os); }

    //    /// Dump the MsgPack into a data stream.
    //    friend std::ostream& operator<<(std::ostream& os, const MsgPack& msgpack);

    //    /// Parse a MsgPack from a data stream.
    //    /// On failure, output a NIL MsgPack and set the stream's failbit.
    //    friend std::istream& operator>>(std::istream& is, MsgPack& msgpack);

private:
    /// Dump the MsgPack into a data stream.
    /// @param depth    How far nested this MsgPack is in relation to the root (used to avoid infinite recursion
    /// @param os       Output data stream to serialize into.
    void _serialize(uint depth, std::ostream& os) const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Value contained in this MsgPack.
    Variant m_value;
};

NOTF_CLOSE_NAMESPACE
