#pragma once

#include <any>

#include "../meta/numeric.hpp"
#include "./common.hpp"

NOTF_OPEN_NAMESPACE

// any helper functions ============================================================================================= //

/// When you have an any value that contains some kind of integral number but you don't know which.
/// @throws std::bad_any_cast   If the any value does not contain an integral number.
template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
T any_integral_cast(const std::any& value)
{
    T result;
    if (value.type() == typeid(bool)) { return static_cast<T>(std::any_cast<bool>(value)); }
    else if (value.type() == typeid(int8_t)) {
        if (can_be_narrow_cast(std::any_cast<int8_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(int16_t)) {
        if (can_be_narrow_cast(std::any_cast<int16_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(int32_t)) {
        if (can_be_narrow_cast(std::any_cast<int32_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(int64_t)) {
        if (can_be_narrow_cast(std::any_cast<int64_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(uint8_t)) {
        if (can_be_narrow_cast(std::any_cast<uint8_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(uint16_t)) {
        if (can_be_narrow_cast(std::any_cast<uint16_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(uint32_t)) {
        if (can_be_narrow_cast(std::any_cast<uint32_t>(value), result)) { return result; }
    }
    else if (value.type() == typeid(uint64_t)) {
        if (can_be_narrow_cast(std::any_cast<uint64_t>(value), result)) { return result; }
    }
    throw std::bad_any_cast();
}

/// When you have an any value that contains some kind of real number but you don't know which.
/// @throws std::bad_any_cast   If the any value does not contain a real number.
template<class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
T any_real_cast(const std::any& value)
{
    if (value.type() == typeid(float)) { return static_cast<T>(std::any_cast<float>(value)); }
    if (value.type() == typeid(double)) { return static_cast<T>(std::any_cast<double>(value)); }

    if (value.type() == typeid(bool)) { return static_cast<T>(std::any_cast<bool>(value)); }
    if (value.type() == typeid(int8_t)) { return static_cast<T>(std::any_cast<int8_t>(value)); }
    if (value.type() == typeid(int16_t)) { return static_cast<T>(std::any_cast<int16_t>(value)); }
    if (value.type() == typeid(int32_t)) { return static_cast<T>(std::any_cast<int32_t>(value)); }
    if (value.type() == typeid(int64_t)) { return static_cast<T>(std::any_cast<int64_t>(value)); }
    if (value.type() == typeid(uint8_t)) { return static_cast<T>(std::any_cast<uint8_t>(value)); }
    if (value.type() == typeid(uint16_t)) { return static_cast<T>(std::any_cast<uint16_t>(value)); }
    if (value.type() == typeid(uint32_t)) { return static_cast<T>(std::any_cast<uint32_t>(value)); }
    if (value.type() == typeid(uint64_t)) { return static_cast<T>(std::any_cast<uint64_t>(value)); }

    throw std::bad_any_cast();
}

NOTF_CLOSE_NAMESPACE
