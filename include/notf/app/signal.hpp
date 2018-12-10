#pragma once

#include "notf/meta/stringtype.hpp"
#include "notf/meta/typename.hpp"

#include "notf/reactive/pipeline.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// any signal ======================================================================================================= //

class AnySignal {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Destructor.
    virtual ~AnySignal() = default;

    /// Name of this Signal type, for runtime reporting.
    virtual std::string_view get_type_name() const = 0;
};

// signal============================================================================================================ //

template<class T>
class Signal : public AnySignal, public Publisher<T, detail::MultiPublisherPolicy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Data type published by the Signal.
    using value_t = T;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    Signal() : Publisher<T, detail::MultiPublisherPolicy>() {}

    /// Name of this Signal type, for runtime reporting.
    std::string_view get_type_name() const final {
        static const std::string my_type = type_name<T>();
        return my_type;
    }
};

// compile time signal ============================================================================================== //

namespace detail {

struct SignalPolicyFactory {

    /// Factory method.
    template<class Policy>
    static constexpr auto create() {

        // validate the given Policy and show an appropriate error message if something goes wrong
        static_assert(decltype(has_value_t<Policy>(std::declval<Policy>()))::value,
                      "A SignalPolicy must contain the type of Signal as type `value_t`");

        static_assert(decltype(has_name<Policy>(std::declval<Policy>()))::value,
                      "A SignalPolicy must contain the name of the Signal as `static constexpr name`");
        static_assert(std::is_same_v<decltype(Policy::name), const StringConst>,
                      "The name of a SignalPolicy must be of type `StringConst`");

        /// Validated CompileTimeSignal policy.
        struct SignalPolicy {

            /// Mandatory value type of the Slot Policy.
            using value_t = typename Policy::value_t;

            /// Mandatory name of the Signal Policy.
            static constexpr const StringConst& get_name() { return Policy::name; }
        };

        return SignalPolicy();
    }

    /// Checks, whether the given type has a nested type `value_t`.
    template<class T>
    static constexpr auto has_value_t(const T&) -> decltype(std::declval<typename T::value_t>(), std::true_type{});
    template<class>
    static constexpr auto has_value_t(...) -> std::false_type;

    /// Checks, whether the given type has a static field `name`.
    template<class T>
    static constexpr auto has_name(const T&) -> decltype(T::name, std::true_type{});
    template<class>
    static constexpr auto has_name(...) -> std::false_type;
};

} // namespace detail

/// Example Policy:
///
///     struct AboutToCloseSignal {
///         using value_t = None;
///         static constexpr StringConst name = "on_about_to_close";
///     };
///
template<class Policy>
class CompileTimeSignal final : public Signal<typename Policy::value_t> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Signal type.
    using policy_t = decltype(detail::SignalPolicyFactory::create<Policy>());

    /// Signal value type.
    using value_t = typename policy_t::value_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// The compile time constant name of this Signal.
    static constexpr const StringConst& get_const_name() noexcept { return policy_t::get_name(); }
};

// signal handle ==================================================================================================== //

/// Object wrapping a weak_ptr to a Slot Subscriber. Is returned by Node::connect_signal and can safely be stored &
/// copied anywhere.
template<class T>
class SignalHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    using signal_t = SignalPtr<T>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param slot     Slot to Handle.
    SignalHandle(const signal_t& signal) : m_signal(signal) {}

    /// Reactive Pipeline "|" operator
    /// Connect the Signal on the left.
    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<signal_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(const SignalHandle& signal, Sub&& subscriber) {
        signal_t publisher = signal.m_signal.lock();
        if (!publisher) { NOTF_THROW(HandleExpiredError, "SignalHandle is expired"); }
        return publisher | std::forward<Sub>(subscriber);
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The handled Signal.
    SignalWeakPtr<T> m_signal;
};

NOTF_CLOSE_NAMESPACE
