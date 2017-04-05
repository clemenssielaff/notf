#pragma once

#include <stddef.h>
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

    const underlying_type id;

    /** Default constructor, produces an invalid ID. */
    Id()
        : id(0) {}

    /** Value constructor. */
    Id(underlying_type id)
        : id(id) {}

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator==(const Other& rhs) const { return id == rhs.id; }

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator!=(const Other& rhs) const { return id != rhs.id; }

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator<(const Other& rhs) const { return id < rhs.id; }

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator<=(const Other& rhs) const { return id <= rhs.id; }

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator>(const Other& rhs) const { return id > rhs.id; }

    template <typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
    bool operator>=(const Other& rhs) const { return id >= rhs.id; }

    explicit operator bool() const { return id != 0; }

    explicit operator underlying_type() const { return id; }
};

} // namespace notf
