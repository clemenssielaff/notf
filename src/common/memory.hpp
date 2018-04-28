#pragma once

#include <memory>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

/// @{
/// Compares two weak_ptrs without locking them.
/// If the two pointers are not of the same type, they are not considered equal.
/// @param a    First weak_ptr.
/// @param b    Second weak_ptr.
template<typename T, typename U>
typename std::enable_if_t<std::is_same<T, U>::value, bool>
weak_ptr_equal(const std::weak_ptr<T>& a, const std::weak_ptr<U>& b)
{
    return !(std::owner_less<std::weak_ptr<T>>()(a, b) || std::owner_less<std::weak_ptr<T>>()(b, a));
}
template<typename T, typename U>
typename std::enable_if_t<(!std::is_same<T, U>::value), bool>
weak_ptr_equal(const std::weak_ptr<T>&, const std::weak_ptr<U>&)
{
    return false;
}
/// @}

NOTF_CLOSE_NAMESPACE
