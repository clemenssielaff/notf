#pragma once

#include <any>

#include "notf/meta/numeric.hpp"

NOTF_OPEN_NAMESPACE

// any helper functions ============================================================================================= //

/// Some types (integrals and reals) will fail `std::any_cast` if you don't have the *exact* right type.
/// Ideally, you would be able to `std::any_cast` to any convertible type, but since that information would be required
/// at compile time, the best we can do is supply it by hand.
/// @throws std::bad_any_cast   If the any value does not contain an integral number.
template<class T, class Any, class = std::enable_if_t<std::is_same_v<Any, std::any>>>
T fuzzy_any_cast(Any&& any)
{
    // integer
    if constexpr (std::is_integral_v<T>) {
        T result;
        if (any.type() == typeid(bool)) { return static_cast<T>(std::any_cast<bool>(std::forward<Any>(any))); }
        else if (any.type() == typeid(int8_t)) {
            if (can_be_narrow_cast(std::any_cast<int8_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(int16_t)) {
            if (can_be_narrow_cast(std::any_cast<int16_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(int32_t)) {
            if (can_be_narrow_cast(std::any_cast<int32_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(int64_t)) {
            if (can_be_narrow_cast(std::any_cast<int64_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(uint8_t)) {
            if (can_be_narrow_cast(std::any_cast<uint8_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(uint16_t)) {
            if (can_be_narrow_cast(std::any_cast<uint16_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(uint32_t)) {
            if (can_be_narrow_cast(std::any_cast<uint32_t>(std::forward<Any>(any)), result)) { return result; }
        }
        else if (any.type() == typeid(uint64_t)) {
            if (can_be_narrow_cast(std::any_cast<uint64_t>(std::forward<Any>(any)), result)) { return result; }
        }
    }

    // real
    else if constexpr (std::is_floating_point_v<T>) {
        if (any.type() == typeid(float)) { return static_cast<T>(std::any_cast<float>(std::forward<Any>(any))); }
        if (any.type() == typeid(double)) { return static_cast<T>(std::any_cast<double>(std::forward<Any>(any))); }

        if (any.type() == typeid(bool)) { return static_cast<T>(std::any_cast<bool>(std::forward<Any>(any))); }
        if (any.type() == typeid(int8_t)) { return static_cast<T>(std::any_cast<int8_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(int16_t)) { return static_cast<T>(std::any_cast<int16_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(int32_t)) { return static_cast<T>(std::any_cast<int32_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(int64_t)) { return static_cast<T>(std::any_cast<int64_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(uint8_t)) { return static_cast<T>(std::any_cast<uint8_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(uint16_t)) { return static_cast<T>(std::any_cast<uint16_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(uint32_t)) { return static_cast<T>(std::any_cast<uint32_t>(std::forward<Any>(any))); }
        if (any.type() == typeid(uint64_t)) { return static_cast<T>(std::any_cast<uint64_t>(std::forward<Any>(any))); }
    }

    // anything else
    else {
        return std::any_cast<T>(std::forward<Any>(any));
    }

    // one of the specializations failed
    throw std::bad_any_cast();
}

NOTF_CLOSE_NAMESPACE
