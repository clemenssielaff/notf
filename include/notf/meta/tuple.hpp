#pragma once

#include <functional>
#include <tuple>
#include <variant>

#include "notf/meta/hash.hpp"

NOTF_OPEN_NAMESPACE

// inspection ======================================================================================================= //

/// Template struct to test whether a given type is a tuple or not.
template<class T>
struct is_tuple : std::false_type {};
template<class... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};
template<class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

/// Checks if a given tuple has any elements or not.
template<class T>
struct is_tuple_empty : std::false_type {};
template<>
struct is_tuple_empty<std::tuple<>> : std::true_type {};
template<class Tuple>
inline constexpr bool is_tuple_empty_v = is_tuple_empty<Tuple>::value;

/// Checks if T is one of the types contained in the given tuple.
template<class T, class... Ts>
struct is_one_of_tuple : std::false_type {};
template<class T, class... Ts>
struct is_one_of_tuple<T, std::tuple<Ts...>> {
    static constexpr bool value = is_one_of_v<T, Ts...>;
};
template<class T, class... Ts>
static constexpr bool is_one_of_tuple_v = is_one_of_tuple<T, Ts...>::value;

/// Checks if T is derived from (or the same as) one of the types contained in the given tuple.
template<class T, class... Ts>
struct is_derived_from_one_of_tuple : std::false_type {};
template<class T, class... Ts>
struct is_derived_from_one_of_tuple<T, std::tuple<Ts...>> {
    static constexpr bool value = is_derived_from_one_of_v<T, Ts...>;
};
template<class T, class... Ts>
static constexpr bool is_derived_from_one_of_tuple_v = is_derived_from_one_of_tuple<T, Ts...>::value;

/// Returns the requested type from a Tuple.
/// Fails if the index is out of bounds and supports negative indices.
template<long I, class Tuple>
constexpr auto tuple_element() { // unfortunately, this cannot be tested at runtime and always appears "uncovered"
    if constexpr (I >= 0) {
        if constexpr (I >= std::tuple_size_v<Tuple>) {
            throw 0; // Positive tuple index is out of bounds
        } else {
            return declval<std::tuple_element_t<static_cast<size_t>(I), Tuple>>();
        }
    } else {
        if constexpr (-I > std::tuple_size_v<Tuple>) {
            throw 0; // Negative tuple index is out of bounds
        } else {
            return declval<std::tuple_element_t<std::tuple_size_v<Tuple> - static_cast<size_t>(-I), Tuple>>();
        }
    }
};
template<long I, class Tuple>
using tuple_element_t = decltype(tuple_element<I, Tuple>());

// make tuple unique ================================================================================================ //

template<class...>
struct make_tuple_unique; // declaration
template<class... Ts>
struct make_tuple_unique<std::tuple<Ts...>> { // recursion breaker
    using type = std::tuple<Ts...>;
};
template<class... Ts, class T, class... Tail>
struct make_tuple_unique<std::tuple<Ts...>, T, Tail...> {  // entry for type list / recursion
    using type = std::conditional_t<is_one_of_v<T, Ts...>, //
                                    typename make_tuple_unique<std::tuple<Ts...>, Tail...>::type,
                                    typename make_tuple_unique<std::tuple<Ts..., T>, Tail...>::type>;
};
template<class... Ts>
struct make_tuple_unique<std::tuple<>, std::tuple<Ts...>> { // entry point for single tuple
    using type = typename make_tuple_unique<std::tuple<>, Ts...>::type;
};

/// Transforms a tuple into a corresponding tuple of unique types (order is retained).
template<class... Ts>
using make_tuple_unique_t = typename make_tuple_unique<std::tuple<>, Ts...>::type;

// for each in tuple ================================================================================================ //

/// @{
/// Applies a given function to each element in the tuple.
/// @param tuple    Tuple to iterate over.
/// @param function Function to execute. Must take the tuple element type as first argument.
/// @param args     (optional) Additional arguments passed bound to the 2nd to nth parameter of function.
template<size_t I = 0, class Function, class... Ts, class... Args>
constexpr std::enable_if_t<(I == sizeof...(Ts))> for_each(std::tuple<Ts...>&, Function&&, Args&&...) noexcept {
} // recursion breaker
template<size_t I = 0, class Function, class... Ts, class... Args>
constexpr std::enable_if_t<(I < sizeof...(Ts))>
for_each(std::tuple<Ts...>& tuple, Function&& function,
         Args&&... args) noexcept(noexcept(std::invoke(function, std::get<I>(tuple), args...))) {
    std::invoke(function, std::get<I>(tuple), args...);
    for_each<I + 1>(tuple, std::forward<Function>(function), std::forward<Args>(args)...);
}

template<size_t I = 0, class Function, class... Ts, class... Args>
constexpr std::enable_if_t<(I == sizeof...(Ts))> for_each(const std::tuple<Ts...>&, Function&&, Args&&...) noexcept {
} // recursion breaker
template<size_t I = 0, class Function, class... Ts, class... Args>
constexpr std::enable_if_t<(I < sizeof...(Ts))>
for_each(const std::tuple<Ts...>& tuple, Function&& function,
         Args&&... args) noexcept(noexcept(std::invoke(function, std::get<I>(tuple), args...))) {
    std::invoke(function, std::get<I>(tuple), args...);
    for_each<I + 1>(tuple, std::forward<Function>(function), std::forward<Args>(args)...);
}
/// @}

// visit at ========================================================================================================= //

namespace detail {

template<size_t Counter, size_t I = Counter - 1>
struct visit_at_impl {
    template<class Tuple, class Function, class... Args>
    static constexpr void visit(const Tuple& tuple, const size_t index, Function fun, Args&&... args) noexcept(
        noexcept(std::invoke(fun, std::get<I>(tuple), std::forward<Args>(args)...))
        && noexcept(visit_at_impl<I>::visit(tuple, index, fun, std::forward<Args>(args)...))) {
        if (index == (I)) {
            std::invoke(fun, std::get<I>(tuple), std::forward<Args>(args)...);
        } else {
            visit_at_impl<I>::visit(tuple, index, fun, std::forward<Args>(args)...);
        }
    }

    template<class Result, class Tuple, class Function, class... Args>
    static constexpr Result visit(const Tuple& tuple, const size_t index, Function fun, Args&&... args) noexcept(
        noexcept(std::invoke(fun, std::get<I>(tuple), std::forward<Args>(args)...))
        && noexcept(visit_at_impl<I>::template visit<Result>(tuple, index, fun, std::forward<Args>(args)...))) {
        if (index == I) {
            return std::invoke(fun, std::get<I>(tuple), std::forward<Args>(args)...);
        } else {
            return visit_at_impl<I>::template visit<Result>(tuple, index, fun, std::forward<Args>(args)...);
        }
    }
};

template<>
struct visit_at_impl<0> {
    template<class Tuple, class Function, class... Args>
    static constexpr void visit(const Tuple&, const size_t, Function, Args&&...) noexcept {}

    template<class Result, class Tuple, class Function, class... Args>
    static constexpr Result visit(const Tuple&, const size_t, Function,
                                  Args&&...) noexcept(std::is_nothrow_default_constructible_v<Result>) {
        static_assert(std::is_default_constructible_v<Result>,
                      "Explicit return type of visit_at method must be default-constructible");
        return {};
    }
};

} // namespace detail

/// @{
/// Applies a given function to a single element of the tuple, identified by index at runtime.
/// If you expect a return type from the function, you'll have to manually declare it as a template argument, because we
/// cannot infer the tuple element type from a runtime index.
/// Implemented after https://stackoverflow.com/a/47385405
/// @param tuple    Tuple to visit over.
/// @param index    Index to visit at.
/// @param function Function to execute. Must take the tuple element type as first argument.
/// @param args     (optional) Additional arguments passed bound to the 2nd to nth parameter of function.
template<class Function, class... Ts, class... Args, size_t Size = sizeof...(Ts)>
constexpr void visit_at(const std::tuple<Ts...>& tuple, size_t index, Function function, Args&&... args) noexcept(
    noexcept(detail::visit_at_impl<Size>::visit(tuple, index, function, std::forward<Args>(args)...))) {
    detail::visit_at_impl<Size>::visit(tuple, index, function, std::forward<Args>(args)...);
}
template<class Result, class... Ts, class Function, class... Args, size_t Size = sizeof...(Ts)>
constexpr Result
visit_at(const std::tuple<Ts...>& tuple, size_t index, Function function, Args&&... args) noexcept(noexcept(
    detail::visit_at_impl<Size>::template visit<Result>(tuple, index, function, std::forward<Args>(args)...))) {
    return detail::visit_at_impl<Size>::template visit<Result>(tuple, index, function, std::forward<Args>(args)...);
}
// @}

// filtered tuple =================================================================================================== //

namespace detail {

template<class Condition>
struct TupleFilter {

    /// Condition function
    template<class Next, class... Ts, class... Tail>
    static constexpr auto
    apply(std::tuple<Ts...>, Next,
          Tail&&... tail) noexcept(noexcept(Condition().template operator()<typename Next::type>())) {
        if constexpr (sizeof...(Tail) > 0) {
            if constexpr (Condition().template operator()<typename Next::type>()) {
                return apply(std::tuple<Ts..., Next>{}, std::forward<Tail>(tail)...);
            } else {
                return apply(std::tuple<Ts...>{}, std::forward<Tail>(tail)...);
            }
        } else { // last loop
            if constexpr (Condition().template operator()<typename Next::type>()) {
                return std::tuple<Ts..., Next>{};
            } else {
                return std::tuple<Ts...>{};
            }
        }
    }

    /// Instead of passing types directly into the condition function, we wrap them into identity<T>.
    /// This way, we can be certain that values of those identity types are default constructible at compile time.
    /// In order to get the original types back, we need to unpack the resulting tuple of identities.
    template<class...>
    struct unbox_types;
    template<class... Ts>
    struct unbox_types<std::tuple<Ts...>> {
        using type = std::tuple<typename Ts::type...>;
    };

    /// A tuple of all types (in order) that pass the condition.
    template<class... Ts>
    using result_t = typename unbox_types<decltype(apply(std::tuple<>{}, identity<Ts>{}...))>::type;
};

} // namespace detail

/// Creates a struct that takes a condition functor and uses it to filter the given types into a tuple.
/// Example:
///
///     struct IsNotEmptyFunctor {
///         template<class T>
///         constexpr bool operator()() noexcept {
///             return !std::is_empty_v<T>; // return true iff T is not an empty type
///         }
///     };
///     template<class... Ts>
///     using remove_empty = create_filtered_tuple<IsNotEmptyFunctor, Ts...>;
///
///     struct Empty {};
///     struct Full { int data; };
///     static_assert(std::is_same_v<
///         remove_empty<Empty, Full, Empty, Empty, Full>,
///         std::tuple<Full, Full>
///     >); // success
///
template<class Condition, class... Ts>
using create_filtered_tuple = typename detail::TupleFilter<Condition>::template result_t<Ts...>;

/// In order to relate an element in a tuple with its place in another tuple (for example one created using
/// `create_filtered_tuple`), this method establishes the "filtered index" of the element with regards to a condition.
/// This way, you can determine that:
///                        this type
///                           v
///     <Full, Empty, Full, Full, Empty, Full, Empty>
/// corresponds to the third entry in the filtered tuple:
///     <Full, Full, Full, Full>
///
/// @param Condition    The condition functor. Must have a operator() implementation with the signature:
///                     `template<class T> constexpr bool operator()() noexcept`
/// @param Index        Index of the element in question in the unfiltered tuple.
/// @param Tuple        The unfiltered tuple to inspect.
template<class Condition, std::size_t Index, class Tuple, std::size_t Current = 0>
constexpr std::size_t get_filtered_tuple_index(const std::size_t result = 0) {

    // one-time checks
    if constexpr (Current == 0) {
        // The index of the element in question must fit into the tuple.
        static_assert(Index < std::tuple_size_v<Tuple>);
        // the element in question must fulfill the condition
        static_assert(Condition().template operator()<std::tuple_element_t<Index, Tuple>>());
    }

    // recursion
    if constexpr (Current < Index) {
        return get_filtered_tuple_index<Condition, Index, Tuple, Current + 1>(
            Condition().template operator()<std::tuple_element_t<Current, Tuple>>() ? result + 1 : result);
    }
    // .. breaker
    else {
        return result;
    }
}

// get first index ================================================================================================== //

/// Finds and returns the first index of the given type in the tuple.
template<class T, class Tuple, size_t I = 0>
constexpr std::enable_if_t<is_tuple_v<Tuple>, size_t> get_first_index() noexcept {
    NOTF_ASSERT(I != std::tuple_size_v<Tuple>);
    if constexpr (std::is_same_v<std::tuple_element_t<I, Tuple>, T>) {
        return I;
    } else {
        return get_first_index<T, Tuple, I + 1>();
    }
}

// count type occurrence ============================================================================================ //

namespace detail {
template<std::size_t I, class Tuple, class... Ts>
constexpr void _count_type_occurrence(std::size_t& count) noexcept {
    if constexpr (I < std::tuple_size_v<Tuple>) {
        if constexpr (is_one_of_v<std::tuple_element_t<I, Tuple>, Ts...>) { ++count; }
        _count_type_occurrence<I + 1, Tuple, Ts...>(count);
    }
}
} // namespace detail

/// Counts the number of times any of the given types appears in the tuple.
/// Example:
///     using test_t = std::tuple<int, bool, int, float, bool, int, int, float>;
///     static_assert(count_type_occurrence<test_t, float, bool>() == 4);
template<class Tuple, class... Ts>
constexpr std::size_t count_type_occurrence() noexcept {
    std::size_t result = 0;
    detail::_count_type_occurrence<0, Tuple, Ts...>(result);
    return result;
}

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

// make n-tuple ===================================================================================================== //

namespace detail {

template<class T, size_t... I>
auto make_n_tuple_impl(std::index_sequence<I...>) {
    return std::tuple<identity_index_t<T, I>...>{};
}

} // namespace detail

/// Tuple type made up of N Ts.
template<class T, size_t N, class Indices = std::make_index_sequence<N>>
using make_n_tuple_t = decltype(detail::make_n_tuple_impl<T>(Indices{}));

static_assert(std::is_same_v<make_n_tuple_t<int, 4>, std::tuple<int, int, int, int>>);

// static tests ===================================================================================================== //

static_assert(
    std::is_same_v<std::tuple<int, float, bool>, make_tuple_unique_t<std::tuple<int, float, int, float, bool, float>>>);
static_assert(std::is_same_v<std::tuple<int, float, bool>, make_tuple_unique_t<int, float, int, float, bool, float>>);

static_assert(
    std::is_same_v<concat_tuple_t<int, std::tuple<bool, double>, float>, std::tuple<int, bool, double, float>>);

static_assert(std::is_same_v<tuple_element_t<0, std::tuple<float, int, bool>>, float>);
static_assert(std::is_same_v<tuple_element_t<1, std::tuple<float, int, bool>>, int>);
static_assert(std::is_same_v<tuple_element_t<2, std::tuple<float, int, bool>>, bool>);
static_assert(std::is_same_v<tuple_element_t<-1, std::tuple<float, int, bool>>, bool>);
static_assert(std::is_same_v<tuple_element_t<-2, std::tuple<float, int, bool>>, int>);
static_assert(std::is_same_v<tuple_element_t<-3, std::tuple<float, int, bool>>, float>);

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

/// std::hash specialization for std::tuple<Ts...>.
template<class... Ts>
struct std::hash<std::tuple<Ts...>> {
    constexpr size_t operator()(const std::tuple<Ts...>& tuple) const {
        std::size_t result = notf::detail::versioned_base_hash();
        _apply_hash_recursively(tuple, result);
        return result;
    }

private:
    template<size_t I = 0>
    constexpr void _apply_hash_recursively(const std::tuple<Ts...>& tuple, size_t& result) const {
        if constexpr (I < sizeof...(Ts)) {
            notf::hash_combine(result, std::get<I>(tuple));
            _apply_hash_recursively<I + 1>(tuple, result);
        }
    }
};
