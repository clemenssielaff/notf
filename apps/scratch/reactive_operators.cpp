#include <exception>
#include <tuple>
#include <type_traits>
#include <variant>

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
///     static_assert(std::is_same_v<typename ft::template arg_type<0>, const int&>); // success
///
template<class Signature>
class function_traits : public function_traits<decltype(&Signature::operator())> {

    // types ----------------------------------------------------------------------------------- //
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
    static constexpr bool has_return_type() noexcept {
        return std::is_same_v<T, return_type>;
    }

    template<size_t index, class T>
    static constexpr bool has_arg_type() noexcept {
        return std::is_same_v<T, std::tuple_element_t<index, args_tuple>>;
    }

    template<class Other, class Indices = std::make_index_sequence<arity>>
    static constexpr bool is_same() noexcept {
        return all(function_traits<Other>::arity == arity,                                    // same arity
                   std::is_same_v<typename function_traits<Other>::return_type, return_type>, // same return type
                   _check_arg_types<Other>(Indices{}));                                       // same argument types
    }

private:
    template<class T, std::size_t index>
    static constexpr bool _check_arg_type() noexcept {
        return std::is_same_v<typename function_traits<T>::template arg_type<index>,
                              typename impl_t::template arg_type<index>>;
    }
    template<class T, std::size_t... i>
    static constexpr bool _check_arg_types(std::index_sequence<i...>) noexcept {
        return (... && _check_arg_type<T, i>());
    }
};
template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...)> { // implementation for class methods
    using return_type = return_t;
    using args_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};
template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...) const> : public function_traits<return_t (class_t::*)(Args...)> {
};
template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...) noexcept>
    : public function_traits<return_t (class_t::*)(Args...)> {};
template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...) const noexcept>
    : public function_traits<return_t (class_t::*)(Args...)> {};
template<class return_t, class... Args>
struct function_traits<return_t (&)(Args...)> { // implementation for free functions
    using return_type = return_t;
    using args_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t I>
    using arg_type = std::tuple_element_t<I, args_tuple>;
};

template<class... Ts>
struct always_false : std::false_type {};
template<class... Ts>
inline constexpr const bool always_false_v = always_false<Ts...>::value;

// ================================================================================================================== //

struct SKIP {};
struct DONE {};

template<class T>
using Next = std::variant<SKIP, // to signal that no new value was generated
                          DONE, // to signal that the operator has completed
                          // std::exception_ptr, // to signal that an exception has occured
                          T>; // a new value

template<class T>
struct is_next : std::false_type {};
template<class T>
struct is_next<Next<T>> : std::true_type {};
template<class T>
inline constexpr const bool is_next_v = is_next<T>::value;

template<class T>
struct next_type {
    static_assert(always_false_v<T>, "Not a Next type");
};
template<class T>
struct next_type<Next<T>> {
    using type = T;
};
template<class T>
using next_t = typename next_type<T>::type;

template<class T>
struct Passthrough {
    Next<T> operator()(T value) noexcept { return {std::move(value)}; }
};

template<class T>
struct AddOne {
    Next<T> operator()(T value) noexcept { return {std::move(value) + 1}; }
};

template<class... Ops>
class Observable {
    static constexpr const std::size_t op_count = sizeof...(Ops);
    static_assert(op_count > 0, "Cannot create an Observable without a single operation");

    using Operations = std::tuple<Ops...>;
    using Data = Operations;

    using input_traits = function_traits<std::tuple_element_t<0, Operations>>;
    static_assert(input_traits::arity == 1);
    using input_t = typename input_traits::template arg_type<0>;

    using output_traits = function_traits<std::tuple_element_t<op_count - 1, Operations>>;
    using output_t = typename output_traits::return_type;
    static_assert(is_next_v<output_t>, "Operators must return a Next<T> value");

    // methods --------------------------------------------------------------------------------- //
public:
    output_t operator()(const input_t& value) {
        try {
            return _call_next<0>(value);
        }
        catch (...) {
            //     return {std::current_exception()};
            return output_t(SKIP());
        }
    }

    // std::exception_ptr

private:
    template<std::size_t I, class T>
    auto _call_next(T value) {
        if constexpr (I < op_count) {
            using result_type = typename function_traits<std::tuple_element_t<I, Operations>>::return_type;
            static_assert(is_next_v<result_type>, "Operators must return a Next<T> value");

            using value_type = next_t<result_type>;
            Next<value_type> result = std::tuple_element_t<I, Operations>()(std::move(value));

            if (std::holds_alternative<value_type>(result)) {
                return _call_next<I + 1>((std::move(std::get<value_type>(result))));
            } else if (std::holds_alternative<SKIP>(result)) {
                return output_t(SKIP());
            } else {
                return output_t(DONE());
            }
        } else {
            return output_t(std::move(value));
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    Data m_data;
};

// int main() {
//    Observable<Passthrough<int>, AddOne<int>> op;
//    int result = std::get<int>(op(3));
//    return result;
//}

// ============================================================================================

#include <tuple>
#include <type_traits>

namespace detail {

template<class T>
struct identity {
    using type = T;
};

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

template<class Condition, class... Ts>
using create_filtered_tuple = typename TupleFilter<Condition>::template result_t<Ts...>;

} // namespace detail

struct IsNotEmptyFunctor {
    template<class T>
    constexpr bool operator()() noexcept {
        return !std::is_empty_v<T>;
    }
};
template<class... Ts>
using remove_empty = detail::create_filtered_tuple<IsNotEmptyFunctor, Ts...>;

struct Empty {
    Empty() = delete;
};
static_assert(std::is_empty_v<Empty>);

struct Full {
    Full() = delete;
    int data;
};
static_assert(!std::is_empty_v<Full>);

using Result = remove_empty<Empty, Full, Empty, Empty, Full>;
static_assert(std::is_same_v<Result, std::tuple<Full, Full>>);

static_assert(std::is_same_v<remove_empty<Empty, Full, Empty, Empty, Full>, std::tuple<Full, Full>>);
// static_assert(std::is_same_v<Result, std::tuple<Empty, Empty, Empty>>);

// ============================================================================================

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
constexpr std::size_t get_filtered_index(const std::size_t result = 0) {

    // one-time checks
    if constexpr (Current == 0) {
        // The index of the element in question must fit into the tuple.
        static_assert(Index < std::tuple_size_v<Tuple>);
        // the element in question must fulfill the condition
        static_assert(Condition().template operator()<std::tuple_element_t<Index, Tuple>>());
    }

    // recursion
    if constexpr (Current < Index) {
        return get_filtered_index<Condition, Index, Tuple, Current + 1>(
            Condition().template operator()<std::tuple_element_t<Current, Tuple>>() ? result + 1 : result);
    }
    // .. breaker
    else {
        return result;
    }
}

static_assert(get_filtered_index<IsNotEmptyFunctor, 0, std::tuple<Full, Full, Full>>() == 0);

int main() { return 0; }
