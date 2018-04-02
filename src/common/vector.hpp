#pragma once

#include <algorithm>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

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
    return std::next(vector.cbegin(), offset);
}
template<typename T>
constexpr typename std::vector<T>::iterator iterator_at(std::vector<T>& vector, size_t offset)
{
    return std::next(vector.begin(), offset);
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
        notf_throw(out_of_bounds, "Cannot take last entry of an empty vector");
    }
    T result = v.back();
    v.pop_back();
    return result;
}

/// Finds the index of a given value in vector.
/// @param vector   Vector to search in.
/// @param value    Value to look for.
/// @param result   [out] Will contain the index, if `value` was found in the vector.
/// @returns        True, iff the value was found - false otherwise.
template<typename T>
bool index_of(const std::vector<T>& vector, const T& value, size_t& result)
{
    for (size_t i = 0, end = vector.size(); i < end; ++i) {
        if (vector[i] == value) {
            result = i;
            return true;
        }
    }
    return false;
}

/// Moves the value at the iterator to the end of the vector.
/// @param vec  Vector to modify.
/// @param it   Iterator to the element to move to the back.
template<typename T>
inline void move_to_back(std::vector<T>& vec, typename std::vector<T>::iterator it)
{
    std::rotate(it, std::next(it), vec.end());
}

/// Moves the value at the iterator to the end of the vector.
/// @param vec  Vector to modify.
/// @param it   Iterator to the element to move to the back.
template<typename T>
inline void move_to_front(std::vector<T>& vec, typename std::vector<T>::iterator it)
{
    std::rotate(vec.begin(), it, std::next(it));
}

/// Stacks the given iterator right before the other.
/// @param vec      Vector to modify.
/// @param it       Iterator to the element to move.
/// @param other    Element following `it` after this function.
template<typename T>
inline void
stack_before(std::vector<T>& vec, typename std::vector<T>::iterator it, typename std::vector<T>::iterator other)
{
    if (it == other) {
        return;
    }
    else if (std::distance(vec.begin(), it) > std::distance(vec.begin(), other)) {
        std::rotate(other, it, std::next(it));
    }
    else {
        std::rotate(it, std::next(it), other);
    }
}

/// Stacks the given iterator right behind the other.
/// @param vec      Vector to modify.
/// @param it       Iterator to the element to move.
/// @param other    Element following `it` after this function.
template<typename T>
inline void
stack_behind(std::vector<T>& vec, typename std::vector<T>::iterator it, typename std::vector<T>::iterator other)
{
    if (it == other) {
        return;
    }
    else if (std::distance(vec.begin(), it) > std::distance(vec.begin(), other)) {
        std::rotate(next(other), it, std::next(it));
    }
    else {
        std::rotate(it, std::next(it), std::next(other));
    }
}

NOTF_CLOSE_NAMESPACE
