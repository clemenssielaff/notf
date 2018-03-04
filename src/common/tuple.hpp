#pragma once

#include <tuple>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE
namespace detail {

template<typename FUNC, typename TUPLE, std::size_t... I>
auto apply_impl(FUNC&& f, TUPLE&& t, std::index_sequence<I...>)
{
    return std::forward<FUNC>(f)(std::get<I>(std::forward<TUPLE>(t))...);
}

} // namespace detail
NOTF_CLOSE_NAMESPACE

#ifndef NOTF_CPP17

namespace std {

/// Expands (applies) a tuple to arguments for a function call.
/// Is included in the std from C++17 onwards.
///
/// From http://stackoverflow.com/a/19060157/3444217
/// but virtually identical to reference implementation from: http://en.cppreference.com/w/cpp/utility/apply
template<typename FUNC, typename TUPLE>
auto apply(FUNC&& f, TUPLE&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<TUPLE>>::value>;
    return notf::detail::apply_impl(std::forward<FUNC>(f), std::forward<TUPLE>(t), Indices());
}

} // namespace std

#endif // #ifndef NOTF_CPP17

NOTF_OPEN_NAMESPACE

/// Checks if the given type is one of the types of the tuple.
template<typename TYPE, typename TUPLE>
struct is_one_of_tuple {
    template<std::size_t... I>
    static constexpr auto impl(std::index_sequence<I...>)
    {
        return is_one_of<TYPE, typename std::tuple_element<I, TUPLE>::type...>{};
    }
    static constexpr auto value = impl(std::make_index_sequence<std::tuple_size<std::decay_t<TUPLE>>::value>{});
};

NOTF_CLOSE_NAMESPACE
