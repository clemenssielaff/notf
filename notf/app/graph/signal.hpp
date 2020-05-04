#pragma once

#include "notf/meta/concept.hpp"
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

// typed signal ===================================================================================================== //

template<class T>
class TypedSignal : public AnySignal, public Publisher<T, detail::MultiPublisherPolicy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Data type published by the Signal.
    using value_t = T;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    TypedSignal() : Publisher<T, detail::MultiPublisherPolicy>() {}

    /// Name of this Signal type, for runtime reporting.
    std::string_view get_type_name() const final {
        static const std::string my_type = type_name<T>();
        return my_type;
    }
};

// signal =========================================================================================================== //

namespace detail {

/// Validates a Signal policy and completes partial policies.
template<class Policy>
class SignalPolicyFactory {

    NOTF_CREATE_TYPE_DETECTOR(value_t);
    static_assert(_has_value_t_v<Policy>, "A SignalPolicy must contain the type of Signal as type `value_t`");

    NOTF_CREATE_FIELD_DETECTOR(name);
    static_assert(_has_name_v<Policy>, "A SignalPolicy must contain the name of the Signal as `static constexpr name`");
    static_assert(std::is_same_v<decltype(Policy::name), const ConstString>,
                  "The name of a SignalPolicy must be of type `ConstString`");

public:
    /// Validated and completed Signal policy.
    struct SignalPolicy {

        /// Mandatory value type of the Signal Policy.
        using value_t = typename Policy::value_t;

        /// Mandatory name of the Signal Policy.
        static constexpr const ConstString name = Policy::name;
    };
};

} // namespace detail

/// Example Policy:
///
///     struct AboutToCloseSignal {
///         using value_t = None;
///         static constexpr ConstString name = "on_about_to_close";
///     };
///
template<class Policy>
class Signal final : public TypedSignal<typename Policy::value_t> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Signal type.
    using policy_t = typename detail::SignalPolicyFactory<Policy>::SignalPolicy;

    /// Signal value type.
    using value_t = typename policy_t::value_t;

    /// The compile time constant name of this Signal.
    static constexpr const ConstString& name = policy_t::name;
};

// signal handle ==================================================================================================== //

/// Object wrapping a weak_ptr to a Signal Publisher. Is returned by Node::connect_signal and can safely be stored and
/// copied anywhere.
template<class T>
class SignalHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    using signal_t = TypedSignalPtr<T>;

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
    TypedSignalWeakPtr<T> m_signal;
};

NOTF_CLOSE_NAMESPACE
