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
    /// Nil value.
    static constexpr auto const Nil = None{};

    /// Boolean
    using Bool = bool;

    /// Default integer type.
    using Int = int32_t;

    /// Default real type.
    using Real = double;

    /// Unicode string.
    using String = std::string;

    /// Binary array.
    using Binary = std::vector<char>;

    /// Array of MsgPack objects.
    using Array = std::vector<MsgPack>;

    /// Map of MsgPack object -> MsgPack object
    using Map = std::map<MsgPack, MsgPack>;

    /// MsgPack extension object.
    using Extension = std::pair<uint8_t, Binary>;

    /// All data types that can be stored in the MsgPack
    enum Type {
        NIL,
        BOOL,
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        FLOAT,
        DOUBLE,
        STRING,
        BINARY,
        ARRAY,
        MAP,
        EXTENSION,
    };

private:
    /// All integer types.
    using int_ts = std::tuple<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

    /// All real types.
    using real_ts = std::tuple<float, double>;

    /// All MsgPack value types.
    using value_ts = concat_tuple_t<None, Bool, int_ts, real_ts, String, Binary, Array, Map, identity<Extension>>;

    /// All types returnable by value.
    using simple_value_ts = concat_tuple_t<None, Bool, int_ts, real_ts>;

    /// All types returnable by reference (every type that is not returned by value).
    using container_value_ts = remove_tuple_types_t<value_ts, simple_value_ts>;

    /// Variant type containing all value types.
    using Variant = tuple_to_variant_t<value_ts>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default constructor, constructs a "None" MsgPack.
    MsgPack() = default;

    /// Construct a MsgPack with a simple value that can be represented.
    /// @param value    Value to initialize the MsgPack object with.
    template<class T, class = std::enable_if_t<is_one_of_tuple_v<T, simple_value_ts>>>
    MsgPack(T value) : m_value(std::move(value))
    {}

    /// Do not allow pointers as value. They would be cast to bool which is most likely not what you'd want.
    MsgPack(void*) = delete;

    /// Explicit constructor for Strings to avoid them being cast to an Array.
    MsgPack(std::string value) : m_value(String(std::move(value))) {}
    MsgPack(std::string_view value) : m_value(String(std::move(value))) {}
    MsgPack(const char* value) : m_value(String(value)) {}

    /// Constructor for Binary objects.
    /// @param value    Binary object to initialize a MsgPack Binary with.
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

    /// Value Getter.
    /// Has two overloads, one for types returned by value; one for types returned by const reference.
    /// @param success  Is set to true, iff a non-empty value was returned.
    /// @returns        A new T by value.
    template<class T>
    std::enable_if_t<is_one_of_tuple_v<T, simple_value_ts>, T> get(bool& success) const noexcept
    {
        success = true;

        // None
        if constexpr (std::is_same_v<T, None>) {
            if (std::holds_alternative<None>(m_value)) {
                return None{}; // all Nones are the same
            }
        }

        // Bool
        else if constexpr (std::is_same_v<T, Bool>) {
            if (std::holds_alternative<Bool>(m_value)) {
                return std::get<Bool>(m_value);
            }
        }

        // TODO: come to think of it ... it doesn't really make sense to store anything else but u/int_64...
        //       the space is used no matter what because we have larger types in the variant
        //       same goes for doubles / floats

        // Integer
        else if constexpr (is_one_of_tuple_v<T, int_ts>) {
            T result;
            if (std::holds_alternative<int8_t>(m_value)) {
                if (can_be_narrow_cast(std::get<int8_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<int16_t>(m_value)) {
                if (can_be_narrow_cast(std::get<int16_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<int32_t>(m_value)) {
                if (can_be_narrow_cast(std::get<int32_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<int64_t>(m_value)) {
                if (can_be_narrow_cast(std::get<int64_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<uint8_t>(m_value)) {
                if (can_be_narrow_cast(std::get<uint8_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<uint16_t>(m_value)) {
                if (can_be_narrow_cast(std::get<uint16_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<uint32_t>(m_value)) {
                if (can_be_narrow_cast(std::get<uint32_t>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<uint64_t>(m_value)) {
                if (can_be_narrow_cast(std::get<uint64_t>(m_value), result)) {
                    return result;
                }
            }
        }

        // Real
        else if constexpr (is_one_of_tuple_v<T, real_ts>) {
            T result;
            if (std::holds_alternative<float>(m_value)) {
                if (can_be_narrow_cast(std::get<float>(m_value), result)) {
                    return result;
                }
            }
            else if (std::holds_alternative<double>(m_value)) {
                if (can_be_narrow_cast(std::get<double>(m_value), result)) {
                    return result;
                }
            }
        }

        // return empty value
        success = false;
        static_assert(std::is_nothrow_constructible_v<T>);
        return {};
    }

    /// Get a const-ref to the requested type or to a default-constructed value.
    /// @param success  Is set to true, iff a non-empty value was returned.
    /// @returns        Const-reference to T.
    template<class T>
    std::enable_if_t<is_one_of_tuple_v<T, container_value_ts>, const T&> get(bool& success) const noexcept
    {
        if (std::holds_alternative<T>(m_value)) {
            success = true;
            return std::get<T>(m_value);
        }

        // return empty value
        success = false;
        static_assert(std::is_nothrow_constructible_v<T>);
        static const T empty;
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

    /// The data type currently held by this MsgPack.
    Type get_type() const noexcept { return static_cast<Type>(m_value.index()); }

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
