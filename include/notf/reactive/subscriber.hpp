#pragma once

#include "../meta/pointer.hpp"
#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// subscriber base ================================================================================================== //

/// Base class for all Subscribers.
struct UntypedSubscriber {

    NOTF_NO_COPY_OR_ASSIGN(UntypedSubscriber);

    /// Default Constructor.
    UntypedSubscriber() = default;

    /// Virtual Destructor.
    virtual ~UntypedSubscriber() = default;

    /// Default implementation of the "error" operation: throws the given exception.
    /// Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
    /// propagate all the way through the callstack, as this could stop the connected Publisher from delivering any more
    /// messages to other Subscribers.
    /// @param exception    The exception that has occurred.
    virtual void on_error(const UntypedPublisher* /*publisher*/, const std::exception& exception) { throw exception; }

    /// Default implementation of the "complete" operation does nothing.
    virtual void on_complete(const UntypedPublisher* /*publisher*/) {}
};

// subscriber ======================================================================================================= //

/// The basic data Subscriber.
template<class T>
class Subscriber : public UntypedSubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type to receive.
    using input_t = T;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param value        Published value.
    virtual void on_next(const UntypedPublisher* publisher, const input_t& value) = 0;
};

/// Subscriber specialization for Subscribers that do not take any data, just signals.
template<>
class Subscriber<NoData> : public UntypedSubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type to receive.
    using input_t = NoData;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    virtual void on_next(const UntypedPublisher* publisher) = 0;
};

// subscriber identifier ============================================================================================ //

template<class T,                                                   // T is a PublisherPtr if it:
         class = std::enable_if_t<is_shared_ptr_v<T>>,              // - is a shared_ptr
         class I = typename T::element_type::input_t>               // - has an input type
struct is_subscriber : std::is_convertible<T, SubscriberPtr<I>> {}; // - can be converted to a PublisherPtr

/// constexpr boolean that is true only if T is a SubscriberPtr
template<class T, class = void>
constexpr bool is_subscriber_v = false;
template<class T>
constexpr bool is_subscriber_v<T, decltype(is_subscriber<T>(), void())> = is_subscriber<T>::value;

NOTF_CLOSE_NAMESPACE
