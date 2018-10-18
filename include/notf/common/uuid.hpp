#pragma once

#include <array>
#include <string>
#include <vector>

#include "notf/meta/hash.hpp"
#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// uuid ============================================================================================================= //

class Uuid {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Internal representation of a UUID.
    using Bytes = std::array<uchar, 16>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default, zero constructor.
    Uuid() : m_bytes({}) {}

    /// Value Constructor.
    /// @param bytes    Bytes to construct the UUID with.
    Uuid(Bytes bytes) : m_bytes(bytes) {}

    /// Value Constructor.
    /// If the string does not contain a valid UUID, the resulting UUID is null.
    /// The valid format is:
    ///     xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    /// Where `x` is a hexadecimal value (0-9, a-f). Any characters beyond are ignored.
    /// @param string   String to initialize the UUID with.
    Uuid(std::string_view string);
    Uuid(const std::string& string) : Uuid(std::string_view(string)) {}
    Uuid(const char* string) : Uuid(std::string_view(string)) {}

    /// Value Constructor.
    /// Takes any vector of size 16+ containing integral values and static casts the values to uchar to initialize the
    /// UUID from them. Produces a null UUID on error.
    /// @param vector   Vector containing integral data to be cast to uchar.
    template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
    Uuid(const std::vector<T>& vector) : m_bytes(_vector_to_bytes(vector))
    {}

    /// Generates a new, valid UUID.
    static Uuid generate();

    /// Checks if this Uuid is anything but all zeros.
    bool is_null() const noexcept { return m_bytes == Bytes{}; }

    /// Access to the internal representation of the Uuid.
    const Bytes& get_data() const noexcept { return m_bytes; }

    /// Packs the Uuid into two size_t words.
    std::pair<size_t, size_t> to_words() const noexcept
    {
        static constexpr auto byte_width = bitsizeof<uchar>();
        size_t first = 0;
        size_t second = 0;
        for (size_t i = 0; i < byte_width; ++i) {
            const size_t shift = byte_width * (byte_width - (i + 1));
            first |= static_cast<size_t>(m_bytes[i]) << shift;
            second |= static_cast<size_t>(m_bytes[i + byte_width]) << shift;
        }
        return std::make_pair(first, second);
    }

    /// Explicit conversion to std::string.
    std::string to_string() const;

    /// Explicit conversion to a vector of T.
    template<class T = char, class = std::enable_if_t<std::is_integral_v<T>>>
    std::vector<T> to_vector() const
    {
        return std::vector<T>(m_bytes.begin(), m_bytes.end());
    }

    /// Comparison operator.
    /// @param other    Other Uuid object to compare against.
    bool operator==(const Uuid other) const noexcept { return m_bytes == other.m_bytes; }

    /// Less-than operator.
    /// @param other    Other Uuid object to compare against.
    bool operator<(const Uuid other) const noexcept { return m_bytes < other.m_bytes; }

    /// Inequality operator.
    /// @param other    Other Uuid object to compare against.
    bool operator!=(const Uuid other) const noexcept { return !(*this == other); }

    /// Less-or-equal-than operator.
    /// @param other    Other Uuid object to compare against.
    bool operator<=(const Uuid other) const noexcept { return !(other < *this); }

    /// Larger-than operator.
    /// @param other    Other Uuid object to compare against.
    bool operator>(const Uuid other) const noexcept { return (other < *this); }

    /// Larger-or-equal-than operator.
    /// @param other    Other Uuid object to compare against.
    bool operator>=(const Uuid other) const noexcept { return !(*this < other); }

    /// Checks if this Uuid is anything but all zeros.
    explicit operator bool() const noexcept { return !is_null(); }

    /// Implicit converstion to std::string.
    operator std::string() const { return to_string(); }

    /// Implicit conversion to a vector of T.
    template<class T = char, class = std::enable_if_t<std::is_integral_v<T>>>
    operator std::vector<T>() const
    {
        return to_vector<T>();
    }

private:
    /// Transforms a vector into a UUID byte array or returns null on error.
    /// @param vector   Vector containing integral data to be cast to uchar.
    /// @returns        Corresponding UUID bytes array.
    template<class T>
    static Bytes _vector_to_bytes(const std::vector<T>& vector)
    {
        if (vector.size() < 16) { return {}; }
        Bytes result;
        for (uchar i = 0; i < 16; ++i) {
            result[i] = static_cast<typename Bytes::value_type>(vector[i]);
        }
        return result;
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Byte representation of this UUID.
    Bytes m_bytes;
};

/// Dump the Uuid into an ostream in human-readable form.
std::ostream& operator<<(std::ostream& os, const Uuid& uuid);

NOTF_CLOSE_NAMESPACE

/// Hash
namespace std {
template<>
struct hash<notf::Uuid> {
    size_t operator()(const notf::Uuid uuid) const noexcept
    {
        auto words = uuid.to_words();
        return notf::hash(words.first, words.second);
    }
};
} // namespace std
