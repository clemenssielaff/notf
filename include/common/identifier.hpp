#pragma once

#include <type_traits>

namespace notf {

/** Strongly typed integral identifier.
 * Is useful if you have multiple types of identifiers that are all the same underlying type and don't want them to be
 * assigneable / comparable.
 */
template <typename Type, typename underlying_type,
          typename = std::enable_if_t<std::is_integral<underlying_type>::value>>
struct Id {
    using type_t = Type;
    using underlying_t = underlying_type;

    /** Invalid Id. */
    static const underlying_type INVALID = 0;

    /** Default constructor, produces an invalid ID. */
    Id()
        : id(INVALID) {}

    /** Value constructor. */
    Id(underlying_type id)
        : id(id) {}

    /** Equal operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator==(const Other& rhs) const { return id == rhs.id; }

    bool operator==(const underlying_t& rhs) const { return id == rhs; }

    /** Not-equal operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator!=(const Other& rhs) const { return id != rhs.id; }

    /** Lesser-then operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator<(const Other& rhs) const { return id < rhs.id; }

    /** Lesser-or-equal-then operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator<=(const Other& rhs) const { return id <= rhs.id; }

    /** Greater-then operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator>(const Other& rhs) const { return id > rhs.id; }

    /** Greater-or-equal-then operator. */
    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator>=(const Other& rhs) const { return id >= rhs.id; }

    /** Checks if this Id is valid or not. */
    explicit operator bool() const { return id != INVALID; }

    /** Cast back to the underlying type, must be explicit to avoid comparison between different Id types. */
    explicit operator underlying_type() const { return id; }

    /** Identifier value of this instance. */
    underlying_type id;
};

} // namespace notf
