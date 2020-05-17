#pragma once

#include <tuple>

#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// ref qualifiers =================================================================================================== //

enum class RefQualifier {
    NONE,    //<
    L_VALUE, //< &
    R_VALUE, //< &&
};

// function traits ================================================================================================== //

/// Helper struct to extract information about an unknown callable (like a lambda) at compile time.
/// From https://stackoverflow.com/a/47699977
/// Example:
///
///     auto lambda = [](const int& i) { return i == 1; };
///     using ft = function_traits<decltype(lambda)>;
///     static_assert (std::is_same_v<ft::return_t, bool>);
///     static_assert (!std::is_same_v<ft::return_t, float>);
///
template<class>
struct function_traits;

// implementation for free functions
template<class ReturnType, class... Args>
struct function_traits<ReturnType (&)(Args...)> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_noexcept = false;
};

// implementation for lambdas
template<class Lambda>
struct function_traits : function_traits<decltype(&Lambda::operator())> {
private:
    using super_t = function_traits<decltype(&Lambda::operator())>;

public:
    using return_t = typename super_t::return_t;
    using arg_ts = typename super_t::arg_ts;
    static constexpr auto is_noexcept = super_t::is_noexcept;
};

// implementations for class methods
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...)> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::NONE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::NONE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::NONE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::NONE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...)&> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::L_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const&> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::L_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...)& noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::L_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const& noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::L_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) &&> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::R_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const&&> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = false;
    static constexpr RefQualifier ref_qualifier = RefQualifier::R_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...)&& noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::R_VALUE;
};
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType (Class::*)(Args...) const&& noexcept> {
    using return_t = ReturnType;
    using arg_ts = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = true;
    static constexpr RefQualifier ref_qualifier = RefQualifier::R_VALUE;
};

// method cast ====================================================================================================== //

namespace detail {
template<class Method>
struct MethodCaster {

    using traits = function_traits<Method>;

    template<class T, std::size_t... Is>
    static constexpr auto _produce_type(std::index_sequence<Is...>) noexcept {
        if constexpr (traits::is_const) {
            if constexpr (traits::ref_qualifier == RefQualifier::R_VALUE) {
                // const&&
                struct container {
                    using type
                        = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...) const&&;
                };
                return container{};
            } else if constexpr (traits::ref_qualifier == RefQualifier::L_VALUE) {
                // const&
                struct container {
                    using type
                        = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...) const&;
                };
                return container{};
            } else {
                // const
                struct container {
                    using type
                        = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...) const;
                };
                return container{};
            }
        } else {
            if constexpr (traits::ref_qualifier == RefQualifier::R_VALUE) {
                // &&
                struct container {
                    using type
                        = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...) &&;
                };
                return container{};
            } else if constexpr (traits::ref_qualifier == RefQualifier::L_VALUE) {
                // &
                struct container {
                    using type
                        = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...) &;
                };
                return container{};
            } else {
                //
                struct container {
                    using type = typename traits::return_t (T::*)(std::tuple_element_t<Is, typename traits::arg_ts>...);
                };
                return container{};
            }
        }
    }

    template<class T, class Indices = std::make_index_sequence<std::tuple_size_v<typename traits::arg_ts>>>
    using cast = typename decltype(_produce_type<T>(Indices{}))::type;
};

} // namespace detail

/// Cast a method function pointer to an equivalent function pointer of another class.
///
/// When handling function pointers that can be accessed through a derived class but are declared in a base class,
/// the type of the method's class will be the base class.
/// This is correct and all but some libraries, for example pybind11, trips over that and throws runtime errors in
/// Python complaining that the `self` argument is of the wrong type (it is of the derived, not the base class).
/// This function can be used to trick pybind into thinking that a method defined in a base class is actually part of
/// the derived class.
template<class T, class Method>
constexpr auto method_cast(Method method) noexcept {
    return static_cast<typename detail::MethodCaster<Method>::template cast<T>>(method);
}

NOTF_CLOSE_NAMESPACE
