#pragma once

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

// signal handle ==================================================================================================== //

/// Object wrapping a weak_ptr to a Slot Subscriber. Is returned by Node::get_signal and can safely be stored & copied
/// anywhere.
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

    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<signal_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(SignalHandle& signal, Sub&& subscriber) {
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
