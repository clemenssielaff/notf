#pragma once

#include <vector>
#include <algorithm>

namespace untitled {

///
/// \brief Removes all occurences of 'element' from 'vector'.
///
/// \param vector   Vector.
/// \param element  Element.
///
/// \return The number of removed items.
///
template<typename T>
auto remove_all(std::vector<T>& vector, const T& element){
    auto size_before = vector.size();
    vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
    return size_before - vector.size();
}

///
/// \brief Removes the first occurence of 'element' in 'vector'.
///
/// \param vector   Vector.
/// \param element  Element.
///
/// \return True, iff an element was removed.
///
template<typename T>
bool remove_one_unordered(std::vector<T>& vector, const T& element){
    using std::swap;
    auto it = std::find(vector.begin(), vector.end(), element);
    if(it == vector.end()){
        return false;
    }
    std::swap(*it, vector.back());
    vector.pop_back();
    return true;
}

} // namespace untitled
