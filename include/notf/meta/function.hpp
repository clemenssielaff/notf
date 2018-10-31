#pragma once

#include <tuple>

#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// function traits ================================================================================================== //

/// Helper struct to extract information about an unknown callable (like a lambda) at compile time.
/// From https://stackoverflow.com/a/47699977
/// Example:
///
///     auto lambda = [](const int& i) { return i == 1; };
///     using ft = function_traits<decltype(lambda)>;
///
///     static_assert(ft::has_return_type<bool>());         // success
///     static_assert(ft::has_arg_type<0, const int&>());   // success
///     // static_assert(ft::has_arg_type<0, float>());     // fail
///
///     auto okay_other = [](const int&) { return true; };
///     static_assert(ft::is_same<decltype(okay_other)>()); // success
///
///     static_assert(ft::is_same<bool (&)(const int&)>()); // success
///
template<class Signature>
class function_traits : public function_traits<decltype(&Signature::operator())> {

    // types ------------------------------------------------------------------------------------------------------- //
private:
    using impl_t = function_traits<decltype(&Signature::operator())>;

public:
    /// Return type of the function.
    using return_type = typename impl_t::return_type;

    /// Tuple corresponding to the argument types of the function.
    using args_tuple = typename impl_t::args_tuple;

    /// Single argument type by index.
    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;

    /// How many arguments the function expects.
    static constexpr auto arity = impl_t::arity;

    // methods --------------------------------------------------------------------------------- //
public:
    template<class T>
    static constexpr bool has_return_type() noexcept
    {
        return std::is_same_v<T, return_type>;
    }

    template<size_t index, class T>
    static constexpr bool has_arg_type() noexcept
    {
        return std::is_same_v<T, std::tuple_element_t<index, args_tuple>>;
    }

    template<class Other, class Indices = std::make_index_sequence<arity>>
    static constexpr bool is_same() noexcept
    {
        return all(function_traits<Other>::arity == arity,                                    // same arity
                   std::is_same_v<typename function_traits<Other>::return_type, return_type>, // same return type
                   _check_arg_types<Other>(Indices{}));                                       // same argument types
    }

private:
    template<class T, std::size_t index>
    static constexpr bool _check_arg_type() noexcept
    {
        return std::is_same_v<typename function_traits<T>::template arg_type<index>,
                              typename impl_t::template arg_type<index>>;
    }
    template<class T, std::size_t... i>
    static constexpr bool _check_arg_types(std::index_sequence<i...>) noexcept
    {
        return (... && _check_arg_type<T, i>());
    }
};
template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...) const> { // implementation for class methods
    using return_type = return_t;
    using args_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};
template<class return_t, class... Args>
struct function_traits<return_t (&)(Args...)> { // implementation for free functions
    using return_type = return_t;
    using args_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};

NOTF_CLOSE_NAMESPACE
