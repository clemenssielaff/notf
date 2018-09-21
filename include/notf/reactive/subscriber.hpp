#pragma once

#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// subscriber base ================================================================================================== //

namespace detail {

struct SubscriberBase {

    NOTF_NO_COPY_OR_ASSIGN(SubscriberBase);

    /// Default Constructor.
    SubscriberBase() = default;

    /// Virtual Destructor.
    virtual ~SubscriberBase() = default;

    /// Default implementation of the "error" operation: throws the given exception.
    /// Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
    /// propagate all the way through the callstack, as this could stop the connected Publisher from delivering any more
    /// messages to other Subscribers.
    /// @param exception    The exception that has occurred.
    virtual void on_error(const std::exception& exception) { throw exception; }

    /// Default implementation of the "complete" operation does nothing.
    virtual void on_complete() {}
};

} // namespace detail

// subscriber ======================================================================================================= //

/// The basic data Subscriber.
template<class T>
struct Subscriber : public detail::SubscriberBase {

    /// Type to receive.
    using input_t = T;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    virtual void on_next(const input_t& value) = 0;
};

/// Subscriber specialization for Subscribers that do not take any data, just signals.
template<>
struct Subscriber<NoData> : public detail::SubscriberBase {

    /// Type to receive.
    using input_t = NoData;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    virtual void on_next() = 0;
};

NOTF_CLOSE_NAMESPACE
