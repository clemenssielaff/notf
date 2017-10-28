#pragma once

#include <tuple>

namespace notf {

namespace detail {

template<typename FUNC, typename TUPLE, std::size_t... I>
auto _apply_impl(FUNC&& f, TUPLE&& t, std::index_sequence<I...>) {
	return std::forward<FUNC>(f)(std::get<I>(std::forward<TUPLE>(t))...);
}

} // namespace detail

/** Expands (applies) a tuple to arguments for a function call.
 * Is included in the std from C++17 onwards.
 *
 * From http://stackoverflow.com/a/19060157/3444217
 * but virtually identical to reference implementation from: http://en.cppreference.com/w/cpp/utility/apply
 */
template<typename FUNC, typename TUPLE>
auto apply(FUNC&& f, TUPLE&& t) {
	using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<TUPLE>>::value>;
	return detail::_apply_impl(std::forward<FUNC>(f), std::forward<TUPLE>(t), Indices());
}

} // namespace notf
