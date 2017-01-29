#pragma once

#include <algorithm>
#include <vector>

namespace notf {

/// @brief Removes all occurences of 'element' from 'vector'.
///
/// @param vector   Vector.
/// @param element  Element.
///
/// @return The number of removed items.
template <typename T>
auto remove_all(std::vector<T>& vector, const T& element)
{
    auto size_before = vector.size();
    vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
    return size_before - vector.size();
}

/// @brief Removes the first occurence of 'element' in 'vector'.
///
/// @param vector   Vector.
/// @param element  Element.
///
/// @return True, iff an element was removed.
template <typename T>
bool remove_one_unordered(std::vector<T>& vector, const T& element)
{
    using std::swap;
    auto it = std::find(vector.begin(), vector.end(), element);
    if (it == vector.end()) {
        return false;
    }
    std::swap(*it, vector.back());
    vector.pop_back();
    return true;
}

/// @brief Returns a vector of all keys in a map.
///
/// @param map  Map to extract the keys from.
template <template <typename...> class MAP, class KEY, class VALUE>
std::vector<KEY> keys(const MAP<KEY, VALUE>& map)
{
    std::vector<KEY> result;
    result.reserve(map.size());
    for (const auto& it : map) {
        result.emplace_back(it.first);
    }
    return result;
}

/// @brief Returns a vector of all values in a map.
///
/// @param map  Map to extract the values from.
template <template <typename...> class MAP, class KEY, class VALUE>
std::vector<VALUE> values(const MAP<KEY, VALUE>& map)
{
    std::vector<VALUE> result;
    result.reserve(map.size());
    for (const auto& it : map) {
        result.emplace_back(it.second);
    }
    return result;
}

/** Appends a vector to another one of the same type. */
template <typename T>
std::vector<T>& append(std::vector<T>& target, const std::vector<T>& source)
{
    target.insert(std::end(target), std::cbegin(source), std::cend(source));
    return target;
}

} // namespace notf
