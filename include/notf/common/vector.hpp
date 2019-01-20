#pragma once

#include <algorithm>
#include <vector>

#include "notf/meta/exception.hpp"
#include "notf/meta/types.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

// vector functions ================================================================================================= //

/// Removes all occurences of 'element' from 'vector'.
/// @return The number of removed items.
template<class T>
auto remove_all(std::vector<T>& vector, const T& element) {
    auto size_before = vector.size();
    vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
    return size_before - vector.size();
}

/// Removes the first occurence of 'element' in 'vector'.
/// @return  True, iff an element was removed.
template<class T>
bool remove_one_unordered(std::vector<T>& vector, const T& element) {
    auto it = std::find(vector.begin(), vector.end(), element);
    if (it == vector.end()) { return false; }
    *it = std::move(vector.back());
    vector.pop_back();
    return true;
}

/// Returns a vector of all keys in a map.
/// @param map  Map to extract the keys from.
template<template<typename...> class MAP, class KEY, class VALUE>
std::vector<KEY> keys(const MAP<KEY, VALUE>& map) {
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
std::vector<VALUE> values(const MAP<KEY, VALUE>& map) {
    std::vector<VALUE> result;
    result.reserve(map.size());
    for (const auto& it : map) {
        result.emplace_back(it.second);
    }
    return result;
}

/// Syntax sugar for an `emplace_back(...)`, followed by a `back()` reference.
template<class T, typename... Args>
T& create_back(std::vector<T>& target, Args&&... args) {
    target.emplace_back(std::forward<Args>(args)...);
    return target.back();
}

/// Extends a vector with another one of the same type.
/// From https://stackoverflow.com/a/41079085
template<class T>
std::vector<T>& extend(std::vector<T>& vector, const std::vector<T>& extension) {
    vector.insert(std::end(vector), std::cbegin(extension), std::cend(extension));
    return vector;
}
template<class T>
std::vector<T>& extend(std::vector<T>& vector, std::vector<T>&& extension) {
    if (vector.empty()) {
        vector = std::move(extension);
    } else {
        std::move(std::begin(extension), std::end(extension), std::back_inserter(vector));
        extension.clear();
    }
    return vector;
}

/// Convenience function to get an iterator to an item at a given index in a vector. */
template<class T>
constexpr typename std::vector<T>::const_iterator iterator_at(const std::vector<T>& vector, size_t offset) {
    return std::next(vector.cbegin(), offset);
}
template<class T>
constexpr typename std::vector<T>::iterator iterator_at(std::vector<T>& vector, size_t offset) {
    return std::next(vector.begin(), offset);
}

/// Flattens a 2D nested vector into a single one.
/// As seen on: http://stackoverflow.com/a/17299623
template<class T>
std::vector<T> flatten(const std::vector<std::vector<T>>& v) {
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
template<class T>
T take_back(std::vector<T>& v) {
    if (v.empty()) { NOTF_THROW(IndexError, "Cannot take last entry of an empty vector"); }
    T result = v.back();
    v.pop_back();
    return result;
}

/// Finds the index of a given value in vector.
/// @param vector   Vector to search in.
/// @param value    Value to look for.
/// @param result   [out] Will contain the index, if `value` was found in the vector.
/// @returns        True, iff the value was found - false otherwise.
template<class T>
bool index_of(const std::vector<T>& vector, const T& value, size_t& result) {
    for (size_t i = 0, end = vector.size(); i < end; ++i) {
        if (vector[i] == value) {
            result = i;
            return true;
        }
    }
    return false;
}

/// Moves the value at the iterator to the end of the vector.
/// Example:
///
///     std::vector<int> vec = { 0, 1, 2, 3, 4, 5, 6, 7 };
///     auto it = std::find(vec.begin(), vec.end(), 5);
///     move_to_back(vec, it); // vector is now { 0, 1, 2, 3, 4, 6, 7, 5 }
///
/// @param vec  Vector to modify.
/// @param it   Iterator to the element to move to the back.
template<class T, class Iterator = typename std::vector<T>::iterator>
void move_to_back(std::vector<T>& vector, Iterator moved) {
    std::rotate(moved, std::next(moved), vector.end());
}

/// Moves the value at the iterator to the front of the vector.
/// Example:
///
///     std::vector<int> vec = { 0, 1, 2, 3, 4, 5, 6, 7 };
///     auto it = std::find(vec.begin(), vec.end(), 5);
///     move_to_back(vec, it); // vector is now { 5, 0, 1, 2, 3, 4, 6, 7 }
///
/// @param vec  Vector to modify.
/// @param it   Iterator to the element to move to the front.
template<class T, class Iterator = typename std::vector<T>::iterator>
void move_to_front(std::vector<T>& vector, Iterator moved) {
    std::rotate(vector.begin(), moved, std::next(moved));
}

/// Stacks the given iterator just to the left of the other.
/// @param vec      Vector to modify.
/// @param it       Iterator to the element to move.
/// @param other    Element just to the right of `it` after this function has run.
template<class T, class Iterator = typename std::vector<T>::iterator>
void move_in_front_of(std::vector<T>& vector, Iterator moved, Iterator other) {
    if (moved == other) {
        return;
    } else if (std::distance(vector.begin(), moved) > std::distance(vector.begin(), other)) {
        std::rotate(other, moved, std::next(moved));
    } else {
        std::rotate(moved, std::next(moved), other);
    }
}
template<class T>
void move_in_front_of(std::vector<T>& vector, const size_t moved_index, const size_t other_index) {
    move_in_front_of(vector, iterator_at(vector, moved_index), iterator_at(vector, other_index));
}

/// Stacks the given iterator just to the right of the other.
/// @param vec      Vector to modify.
/// @param it       Iterator to the element to move.
/// @param other    Element just to the left of `it` after this function has run.
template<class T, class Iterator = typename std::vector<T>::iterator>
void move_behind_of(std::vector<T>& vector, Iterator moved, Iterator other) {
    if (moved == other) {
        return;
    } else if (std::distance(vector.begin(), moved) > std::distance(vector.begin(), other)) {
        std::rotate(next(other), moved, std::next(moved));
    } else {
        std::rotate(moved, std::next(moved), std::next(other));
    }
}
template<class T>
void move_behind_of(std::vector<T>& vector, const size_t moved_index, const size_t other_index) {
    move_behind_of(vector, iterator_at(vector, moved_index), iterator_at(vector, other_index));
}

/// Checks if a vector contains a given value.
/// @param value    Value to test for.
template<class T>
bool contains(const std::vector<T>& vector, const identity_t<T>& value) {
    return std::find(vector.begin(), vector.end(), value) != vector.end();
}

/// Calls the given function on each valid element and removes the rest.
/// @param vector       Vector to operate on.
/// @param predicate    Condition that an element has to meet in order to be considered valid.
/// @param functor      Function to call on each valid element.
template<class T, class P, class F>
void iterate_and_clean(std::vector<T>& vector, P&& predicate, F&& functor) {
    size_t gap = 0;
    for (size_t index = 0, end = vector.size(); index < end; ++index) {
        if (predicate(vector[index])) {
            functor(vector[index]);
            if (gap != index) { vector[gap] = std::move(vector[index]); }
            ++gap;
        }
    }
    if (gap != vector.size()) { vector.erase(iterator_at(vector, gap), vector.end()); }
}

// reverse iteration ================================================================================================ //

namespace detail {
template<typename T>
struct reversion_wrapper {
    T& iterable;
};
} // namespace detail

template<typename T>
auto begin(detail::reversion_wrapper<T> w) {
    return std::rbegin(w.iterable);
}

template<typename T>
auto end(detail::reversion_wrapper<T> w) {
    return std::rend(w.iterable);
}

/// Reverse iterator adaptor.
/// Use like this:
///     for(const auto& foo : reverse(bar)){
///         cout << foo << endl;
///     }
/// From: http://stackoverflow.com/a/28139075
template<typename T>
detail::reversion_wrapper<T> reverse(T&& iterable) {
    return {iterable};
}

NOTF_CLOSE_NAMESPACE
