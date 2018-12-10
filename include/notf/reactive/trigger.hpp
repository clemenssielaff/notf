#pragma once

#include "notf/meta/function.hpp"

#include "notf/reactive/subscriber.hpp"

NOTF_OPEN_NAMESPACE

// trigger identifier =============================================================================================== //

namespace detail {

template<class Callback, class fn_traits = function_traits<Callback>>
struct TriggerIdentifier {

    static constexpr auto validate_traits() {
        static_assert(std::is_same_v<typename fn_traits::return_type, void>, "A Trigger must return void");
        static_assert((fn_traits::arity >= 0 && fn_traits::arity <= 2),
                      "A Trigger must take zero, one or two arguments");
        if constexpr (fn_traits::arity == 2) {
            static_assert(std::is_same_v<typename fn_traits::template arg_type<0>, const AnyPublisher*>,
                          "If a Trigger takes two arguments, the first one must be of type`const AnyPublisher*`");
        }
        return std::declval<fn_traits>();
    }

    static constexpr auto get_type() {
        if constexpr (fn_traits::arity == 0) {
            return std::declval<None>();
        } else if constexpr (fn_traits::arity == 1) {
            return std::declval<std::decay_t<typename fn_traits::template arg_type<0>>>();
        } else {
            static_assert(fn_traits::arity == 2);
            return std::declval<std::decay_t<typename fn_traits::template arg_type<1>>>();
        }
    }

    template<class X = decltype(validate_traits())>
    using type = decltype(get_type());
};

} // namespace detail

// trigger ========================================================================================================== //

/// A Trigger is a simple helper to conveniently create a Subscriber with a single lambda.
/// @param callback Callback executed whenever the Subscriber receives a new value.
///                 Must be in the form `void(const T&)` or `void(const AnyPublisher*, const T&)`.
template<class Callback, class T = typename detail::TriggerIdentifier<Callback>::template type<>>
auto Trigger(Callback&& callback) {

    // implementation for T == None
    if constexpr (std::is_same_v<T, None>) {
        class TriggerImpl : public Subscriber<None> {

            // methods ----------------------------------------------------------------------------- //
        public:
            /// Constructor.
            TriggerImpl(Callback&& callback) : m_callback(std::forward<Callback>(callback)) {}

            /// Called whenever the Subscriber receives a new value.
            /// @param publisher    The Publisher publishing the value, for identification purposes only.
            /// @param value        Published value.
            void on_next(const AnyPublisher* publisher) final {
                if constexpr (function_traits<Callback>::arity == 0) {
                    std::invoke(m_callback);
                } else {
                    static_assert(function_traits<Callback>::arity == 1);
                    std::invoke(m_callback, publisher);
                }
            }

            // fields ------------------------------------------------------------------------------ //
        private:
            /// Callback executed whenever the Subscriber receives a new value.
            Callback m_callback;
        };
        return std::make_shared<TriggerImpl>(std::forward<Callback>(callback));

    }

    // implementation for T != None
    else {
        class TriggerImpl : public Subscriber<T> {

            // methods ----------------------------------------------------------------------------- //
        public:
            /// Constructor.
            TriggerImpl(Callback&& callback) : m_callback(std::forward<Callback>(callback)) {}

            /// Called whenever the Subscriber receives a new value.
            /// @param publisher    The Publisher publishing the value, for identification purposes only.
            /// @param value        Published value.
            void on_next(const AnyPublisher* publisher, const T& value) final {
                if constexpr (function_traits<Callback>::arity == 2) {
                    std::invoke(m_callback, publisher, value);
                } else {
                    static_assert(function_traits<Callback>::arity == 1);
                    std::invoke(m_callback, value);
                }
            }

            // fields ------------------------------------------------------------------------------ //
        private:
            /// Callback executed whenever the Subscriber receives a new value.
            Callback m_callback;
        };
        return std::make_shared<TriggerImpl>(std::forward<Callback>(callback));
    }
}

NOTF_CLOSE_NAMESPACE
