#pragma once

#include <iterator>

template<typename T>
struct reversion_wrapper {
    T& iterable;
};

template<typename T>
auto begin(reversion_wrapper<T> w)
{
    return std::rbegin(w.iterable);
}

template<typename T>
auto end(reversion_wrapper<T> w)
{
    return std::rend(w.iterable);
}

/** Reverse iterator adaptor.
 * Use like this:
 *     for(const auto& foo : reverse(bar)){
 *         cout << foo << endl;
 *     }
 * From: http://stackoverflow.com/a/28139075/3444217
 */
template<typename T>
reversion_wrapper<T> reverse(T&& iterable)
{
    return {iterable};
}
