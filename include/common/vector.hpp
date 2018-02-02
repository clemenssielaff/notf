#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "common/meta.hpp"

namespace notf {

/// Removes all occurences of 'element' from 'vector'.
/// @return The number of removed items.
template<typename T>
auto remove_all(std::vector<T>& vector, const T& element)
{
    auto size_before = vector.size();
    vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
    return size_before - vector.size();
}

/// Removes the first occurence of 'element' in 'vector'.
/// @return  True, iff an element was removed.
template<typename T>
bool remove_one_unordered(std::vector<T>& vector, const T& element)
{
    auto it = std::find(vector.begin(), vector.end(), element);
    if (it == vector.end()) {
        return false;
    }
    *it = std::move(vector.back());
    vector.pop_back();
    return true;
}

/// Returns a vector of all keys in a map.
/// @param map  Map to extract the keys from.
template<template<typename...> class MAP, class KEY, class VALUE>
std::vector<KEY> keys(const MAP<KEY, VALUE>& map)
{
    std::vector<KEY> result;
    result.reserve(map.size());
    for (const auto& it : map) {
        result.emplace_back(it.first);
    }
    return result;
}

/// Returns a vector of all values in a map.
/// @param map  Map to extract the values from.
template<template<typename...> class MAP, class KEY, class VALUE>
std::vector<VALUE> values(const MAP<KEY, VALUE>& map)
{
    std::vector<VALUE> result;
    result.reserve(map.size());
    for (const auto& it : map) {
        result.emplace_back(it.second);
    }
    return result;
}

#ifndef NOTF_CPP17
/// Syntax sugar for an `emplace_back(...)`, followed by a `back()` reference.
/// Will be standard in C++17.
template<typename T, typename... Args>
T& create_back(std::vector<T>& target, Args&&... args)
{
    target.emplace_back(std::forward<Args>(args)...);
    return target.back();
}
#endif

/// Extends a vector with another one of the same type.
/// From https://stackoverflow.com/a/41079085/3444217
template<typename T>
std::vector<T>& extend(std::vector<T>& vector, const std::vector<T>& extension)
{
    vector.insert(std::end(vector), std::cbegin(extension), std::cend(extension));
    return vector;
}
template<typename T>
std::vector<T>& extend(std::vector<T>& vector, std::vector<T>&& extension)
{
    if (vector.empty()) {
        vector = std::move(extension);
    }
    else {
        std::move(std::begin(extension), std::end(extension), std::back_inserter(vector));
        extension.clear();
    }
    return vector;
}

/// Convenience function to get an iterator to an item at a given index in a vector. */
template<typename T>
constexpr typename std::vector<T>::const_iterator iterator_at(const std::vector<T>& vector, size_t offset)
{
    return vector.cbegin() + static_cast<typename std::vector<T>::difference_type>(offset);
}
template<typename T>
constexpr typename std::vector<T>::iterator iterator_at(std::vector<T>& vector, size_t offset)
{
    return vector.begin() + static_cast<typename std::vector<T>::difference_type>(offset);
}

/// Flattens a 2D nested vector into a single one.
/// As seen on: http://stackoverflow.com/a/17299623/3444217
template<typename T>
std::vector<T> flatten(const std::vector<std::vector<T>>& v)
{
    std::size_t total_size = 0;
    for (const auto& sub : v) {
        total_size += sub.size();
    }
    std::vector<T> result;
    result.reserve(total_size);
    for (const auto& sub : v) {
        result.insert(result.end(), sub.begin(), sub.end());
    }
    return result;
}

/// Takes and removes the last entry of a vector and returns it.
/// @throws std::out_of_range exception if the vector is empty.
template<typename T>
T take_back(std::vector<T>& v)
{
    if (v.empty()) {
        throw std::out_of_range("Cannot take last entry of an empty vector");
    }
    T result = v.back();
    v.pop_back();
    return result;
}

} // namespace notf
