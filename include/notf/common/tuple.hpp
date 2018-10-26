#pragma once

#include <tuple>

#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// flatten tuple ==================================================================================================== //

// template definition
template<class... Ts>
struct concat_tuple;

// single ground type
template<class T>
struct concat_tuple<T> {
    using type = std::tuple<T>;
};

// single tuple
template<class... Ts>
struct concat_tuple<std::tuple<Ts...>> {
    using type = std::tuple<Ts...>;
};

// single identity type
template<class T>
struct concat_tuple<identity<T>> {
    using type = T;
};

// two ground types
template<class L, class R, class... Tail>
struct concat_tuple<L, R, Tail...> {
    using type = typename concat_tuple<std::tuple<L, R>, Tail...>::type;
};

// two tuples
template<class... Ls, class... Rs, class... Tail>
struct concat_tuple<std::tuple<Ls...>, std::tuple<Rs...>, Tail...> {
    using type = typename concat_tuple<std::tuple<Ls..., Rs...>, Tail...>::type;
};

// two identities
template<class L, class R, class... Tail>
struct concat_tuple<identity<L>, identity<R>, Tail...> {
    using type = typename concat_tuple<std::tuple<L, R>, Tail...>::type;
};

// ground & tuple
template<class L, class... Rs, class... Tail>
struct concat_tuple<L, std::tuple<Rs...>, Tail...> {
    using type = typename concat_tuple<std::tuple<L, Rs...>, Tail...>::type;
};

// tuple & ground
template<class... Ls, class R, class... Tail>
struct concat_tuple<std::tuple<Ls...>, R, Tail...> {
    using type = typename concat_tuple<std::tuple<Ls..., R>, Tail...>::type;
};

// ground & identity
template<class L, class R, class... Tail>
struct concat_tuple<L, identity<R>, Tail...> {
    using type = typename concat_tuple<std::tuple<L, R>, Tail...>::type;
};

// identity & ground
template<class L, class R, class... Tail>
struct concat_tuple<identity<L>, R, Tail...> {
    using type = typename concat_tuple<std::tuple<L, R>, Tail...>::type;
};

// tuple & identity
template<class... Ls, class R, class... Tail>
struct concat_tuple<std::tuple<Ls...>, identity<R>, Tail...> {
    using type = typename concat_tuple<std::tuple<Ls..., R>, Tail...>::type;
};

// identity & tuple
template<class L, class... Rs, class... Tail>
struct concat_tuple<identity<L>, std::tuple<Rs...>, Tail...> {
    using type = typename concat_tuple<std::tuple<L, Rs...>, Tail...>::type;
};

/// Flattens a list of types (that may or may not be other tuples) into a single tuple.
/// Unlike std::tuple_concat, we allow certain tuples to remain "unflattened" if they are wrapped into the `identity`
/// type.
template<class... Ts>
using concat_tuple_t = typename concat_tuple<Ts...>::type;

// inspection ======================================================================================================= //

/// Checks if T is one of the types contained in the given tuple.
template<class T, class... Ts>
struct is_one_of_tuple {
    static constexpr bool value = false;
};
template<class T, class... Ts>
struct is_one_of_tuple<T, std::tuple<Ts...>> {
    static constexpr bool value = is_one_of_v<T, Ts...>;
};
template<class T, class... Ts>
static constexpr bool is_one_of_tuple_v = is_one_of_tuple<T, Ts...>::value;

/// Checks if T is derived from (or the same as) one of the types contained in the given tuple.
template<class T, class... Ts>
struct is_derived_from_one_of_tuple {
    static constexpr bool value = false;
};
template<class T, class... Ts>
struct is_derived_from_one_of_tuple<T, std::tuple<Ts...>> {
    static constexpr bool value = is_derived_from_one_of_v<T, Ts...>;
};
template<class T, class... Ts>
static constexpr bool is_derived_from_one_of_tuple_v = is_derived_from_one_of_tuple<T, Ts...>::value;

/// Returns the requested type from a Tuple.
/// Fails if the index is out of bounds and supports negative indices.
template<long I, class Tuple>
constexpr auto tuple_element()
{
    if constexpr (I >= 0) {
        if constexpr (I >= std::tuple_size_v<Tuple>) {
            throw 0; // Positive tuple index is out of bounds
        }
        else {
            return std::tuple_element_t<static_cast<size_t>(I), Tuple>{};
        }
    }
    else {
        if constexpr (-I > std::tuple_size_v<Tuple>) {
            throw 0; // Negative tuple index is out of bounds
        }
        else {
            return std::tuple_element_t<std::tuple_size_v<Tuple> - static_cast<size_t>(-I), Tuple>{};
        }
    }
};
template<long I, class Tuple>
using tuple_element_t = decltype(tuple_element<I, Tuple>());

// remove tuple types =============================================================================================== //

namespace detail {

// template definition
template<class... Ts>
struct _remove_tuple_types;

// recursion breaker
template<class... Ds, class... Result, class... Tail>
struct _remove_tuple_types<std::tuple<Ds...>, std::tuple<Result...>, Tail...> {

    using type = std::tuple<Result...>;
};

// recursive
template<class... Ds, class... Result, class Head, class... Tail>
struct _remove_tuple_types<std::tuple<Ds...>, std::tuple<Result...>, Head, Tail...> {
    // Note: I tried std::conditional here in place of the decltype(), but clang immediately ran out of memory with a 16
    // element-long tuple ... This works as well and consumes almost no memory.
    static auto constexpr _result_type()
    {
        if constexpr (is_one_of_v<Head, Ds...>) { return std::tuple<Result...>{}; }
        else {
            return std::tuple<Result..., Head>{};
        }
    }
    using type = typename _remove_tuple_types<std::tuple<Ds...>, decltype(_result_type()), Tail...>::type;
};

// first ground type
template<class... Ds, class First, class... Tail>
struct _remove_tuple_types<std::tuple<Ds...>, First, Tail...> {

    static auto constexpr _result_type()
    {
        if constexpr (is_one_of_v<First, Ds...>) { return std::tuple<>{}; }
        else {
            return std::tuple<First>{};
        }
    }
    using type = typename _remove_tuple_types<std::tuple<Ds...>, decltype(_result_type()), Tail...>::type;
};

} // namespace detail

template<class... Ts>
struct remove_tuple_types {
    static_assert(always_false_v<Ts...>);
};

// base tuple & delta tuple
template<class... Ts, class... Ds>
struct remove_tuple_types<std::tuple<Ts...>, std::tuple<Ds...>> {
    using type = typename detail::_remove_tuple_types<std::tuple<Ds...>, Ts...>::type;
};

// base tuple & variadic delta
template<class... Ts, class... Ds>
struct remove_tuple_types<std::tuple<Ts...>, Ds...> {
    using type = typename detail::_remove_tuple_types<std::tuple<Ds...>, Ts...>::type;
};

/// Removes all occurrences of every type in Ts from Tuple and returns a new Tuple type.
template<class... Ts>
using remove_tuple_types_t = typename remove_tuple_types<Ts...>::type;

// static tests ===================================================================================================== //

static_assert(
    std::is_same_v<concat_tuple_t<int, std::tuple<bool, double>, float>, std::tuple<int, bool, double, float>>);

static_assert(std::is_same_v<remove_tuple_types_t<std::tuple<int, float, double, int, bool, int>, int, double>,
                             std::tuple<float, bool>>);

static_assert(
    std::is_same_v<remove_tuple_types_t<std::tuple<int, float, double, int, bool, int>, std::tuple<int, double>>,
                   std::tuple<float, bool>>);

static_assert(std::is_same_v<tuple_element_t<0, std::tuple<float, int, bool>>, float>);
static_assert(std::is_same_v<tuple_element_t<1, std::tuple<float, int, bool>>, int>);
static_assert(std::is_same_v<tuple_element_t<2, std::tuple<float, int, bool>>, bool>);
static_assert(std::is_same_v<tuple_element_t<-1, std::tuple<float, int, bool>>, bool>);
static_assert(std::is_same_v<tuple_element_t<-2, std::tuple<float, int, bool>>, int>);
static_assert(std::is_same_v<tuple_element_t<-3, std::tuple<float, int, bool>>, float>);

NOTF_CLOSE_NAMESPACE
