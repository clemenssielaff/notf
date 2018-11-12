#pragma once

#include <map>
#include <vector>

#include "notf/meta/real.hpp"

#include "notf/common/variant.hpp"

NOTF_OPEN_NAMESPACE

// msgpack ========================================================================================================== //

class MsgPack {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(MsgPack);
    friend AccessFor<MsgPack>;

    /// Data types.
    using None = ::notf::None;
    using Bool = bool;
    using Int = int64_t;
    using Uint = uint64_t;
    using Float = float;
    using Double = double;
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
        FLOAT,
        DOUBLE,
        STRING,
        BINARY,
        ARRAY,
        MAP,
        EXTENSION,
    };

    /// The MsgPack spec allows user-defined extension types with the index [0 -> 127].
    enum class ExtensionType : int8_t {
        UUID = 10,
    };

    /// Generic error thrown when deserialization fails.
    NOTF_EXCEPTION_TYPE(ParseError);

    /// Error thrown during deserialization, if the constructed MsgPack is too deep.
    NOTF_EXCEPTION_TYPE(RecursionDepthExceededError);

private:
    /// All types returnable by value.
    using fundamental_types = std::tuple<None, Bool, Int, Uint, Float, Double>;

    /// All types returnable by reference (every type that is not returned by value).
    using container_types = std::tuple<String, Binary, Array, Map, Extension>;

    /// Variant type containing all value types.
    using Variant = tuple_to_variant_t<concat_tuple_t<fundamental_types, container_types>>;

    // methods --------------------------------------------------------------------------------- //
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
    MsgPack(float value) : m_value(value) {}
    MsgPack(double value) : m_value(value) {}

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

    /// Constructor for Extension objects.
    /// @param value    MsgPack extension object
    MsgPack(MsgPack::Extension value) : m_value(std::move(value)) {}
    MsgPack(ExtensionType type, Binary data) : m_value(std::make_pair(to_number(type), std::move(data))) {}

    /// The data type currently held by this MsgPack.
    Type get_type() const noexcept { return static_cast<Type>(m_value.index()); }

    /// Value Getter.
    /// Has two overloads, one for types returned by value; one for types returned by const reference.
    /// @param success  Is set to true, iff a non-empty value was returned.
    /// @returns        A new T by value.
    template<class T>
    std::enable_if_t<is_one_of_variant<T, Variant>(),
                     std::conditional_t<is_one_of_tuple_v<T, fundamental_types>, T, const T&>>
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
                if (auto value = std::get<Uint>(m_value); value < static_cast<Uint>(max_value<Int>())) {
                    return static_cast<T>(value);
                }
            }
        } else if constexpr (std::is_same_v<T, Uint>) {
            if (std::holds_alternative<Int>(m_value)) {
                if (auto value = std::get<Int>(m_value); value >= 0) { return static_cast<T>(value); }
            }
        }

        // floating point types are interchangeable
        else if constexpr (std::is_floating_point_v<T>) {
            if (std::holds_alternative<Float>(m_value)) {
                return static_cast<T>(std::get<Float>(m_value));
            } else if (std::holds_alternative<Double>(m_value)) {
                return static_cast<T>(std::get<Double>(m_value));
            }

            // even the biggest unsigned integer can be cast to floating point
            else if (std::holds_alternative<Int>(m_value)) {
                return static_cast<T>(std::get<Int>(m_value));
            } else if (std::holds_alternative<Uint>(m_value)) {
                return static_cast<T>(std::get<Uint>(m_value));
            }
        }

        // return empty value
        success = false;
        static_assert(std::is_nothrow_constructible_v<T> //
			|| std::is_same_v<T, Extension> 
			#ifdef NOTF_MSVC
                      || std::is_same_v<T, Map> 
			#endif
			);
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
    /// @throws ValueError     If the MsgPack does not contain an Array.
    /// @throws OutOfBounds   If the index is larger than the largest index in the Array.
    const MsgPack& operator[](const size_t index) const
    {
        if (!std::holds_alternative<Array>(m_value)) { NOTF_THROW(ValueError, "MsgPack object is not an Array"); }

        const auto& array = std::get<Array>(m_value);
        if (index < array.size()) { return array[index]; }

        NOTF_THROW(OutOfBounds, "MsgPack Array has only {} elements, requested index was {}", array.size(), index);
    }

    /// If this MsgPack contains a map, returns the element matching the given string key.
    /// This is a convenience function, if you plan to make extensive use of the map, consider `get`ting the underlying
    /// MsgPack::Map object directly.
    /// @param key              String key of the requested element in the map.
    /// @throws ValueError     If the MsgPack does not contain an Array.
    /// @throws OutOfBounds   If the index is larger than the largest index in the Array.
    const MsgPack& operator[](const std::string& key) const
    {
        if (!std::holds_alternative<Map>(m_value)) { NOTF_THROW(ValueError, "MsgPack object is not a Map"); }

        const auto& map = std::get<Map>(m_value);
        if (auto it = map.find(MsgPack(key)); it != map.end()) { return it->second; }

        NOTF_THROW(OutOfBounds, "MsgPack Map does not contain requested key \"{}\"", key);
    }

    /// Comparison operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator==(const MsgPack& other) const
    {
        const Type my_type = get_type();
        const Type other_type = other.get_type();

        /// T <=> T comparison
        if (my_type == other_type) {
            return m_value == other.m_value;
        }

        /// int <=> uint comparison
        else if ((my_type == Type::INT && other_type == Type::UINT)
                 || (my_type == Type::UINT && other_type == Type::INT)) {
            return get<Int>() == other.get<Int>();
        }

        /// float <=> double comparison
        else if ((my_type == Type::FLOAT && other_type == Type::DOUBLE)
                 || (my_type == Type::DOUBLE && other_type == Type::FLOAT)) {
            return is_approx(get<Double>(), other.get<Double>());
        }

        /// T != !T
        return false;
    }

    /// Less-than operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator<(const MsgPack& other) const { return m_value < other.m_value; }

    /// Inequality operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator!=(const MsgPack& other) const { return !(*this == other); }

    /// Less-or-equal-than operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator<=(const MsgPack& other) const { return !(other < *this); }

    /// Larger-than operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator>(const MsgPack& other) const { return (other < *this); }

    /// Larger-or-equal-than operator.
    /// @param other    Other MsgPack object to compare against.
    bool operator>=(const MsgPack& other) const { return !(*this < other); }

    /// Checks if this MsgPack contains any value type but None.
    explicit operator bool() const noexcept { return !std::holds_alternative<None>(m_value); }

    /// Dump the MsgPack into a data stream.
    /// @param os   Output data stream to serialize into.
    void serialize(std::ostream& os) const { _serialize(os, /*depth=*/0); }

    /// Create a new MsgPack object by deserializing it from a data stream.
    /// @param is   Input data stream to read from.
    static MsgPack deserialize(std::istream& is) { return _deserialize(is, /*depth=*/0); }

private:
    /// Dump the MsgPack into a data stream.
    /// @param os       Output data stream to serialize into.
    /// @param depth    How far nested this MsgPack is in relation to the root (used to avoid infinite recursion
    void _serialize(std::ostream& os, uint depth) const;

    /// Create a new MsgPack object by deserializing it from a data stream.
    /// @param is   Input data stream to read from.
    /// @param depth    How far nested this MsgPack is in relation to the root (used to avoid infinite recursion
    static MsgPack _deserialize(std::istream& is, uint depth);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Value contained in this MsgPack.
    Variant m_value;

    /// Maximum recursion depth while parsing a MsgPack before throwing `RecursionDepthExceededError`.
    inline static uint s_max_recursion_depth = 128;
};

NOTF_CLOSE_NAMESPACE
