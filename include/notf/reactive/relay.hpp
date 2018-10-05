#pragma once

#include "./publisher.hpp"
#include "./subscriber.hpp"

NOTF_OPEN_NAMESPACE

// relay base template ============================================================================================== //

/// Base Relay template, combining a Subscriber and Producer (potentially of different types) into a single object.
template<class I, class O = I, class Policy = detail::DefaultPublisherPolicy>
class Relay : public Subscriber<I>, public Publisher<O, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Relay inherits.
    using subscriber_t = Subscriber<I>;

    /// Publisher type from which this Relay inherits.
    using publisher_t = Publisher<O, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const UntypedPublisher* /*publisher*/, const O& value) override { this->publish(value); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const UntypedPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const UntypedPublisher* /*publisher*/) override { this->complete(); }
};

// NoData relay specializations ===================================================================================== //

/// Specialization for Relays that transmit no data.
template<class Policy>
class Relay<NoData, NoData, Policy> : public Subscriber<NoData>, public Publisher<NoData, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Relay inherits.
    using subscriber_t = Subscriber<NoData>;

    /// Publisher type from which this Relay inherits.
    using publisher_t = Publisher<NoData, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const UntypedPublisher* /*publisher*/) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const UntypedPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const UntypedPublisher* /*publisher*/) override { this->complete(); }
};

/// Specialization for Relays that connect a data-producing upstream to a "NoData" downstream.
template<class T, class Policy>
class Relay<T, NoData, Policy> : public Subscriber<T>, public Publisher<NoData, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Relay inherits.
    using subscriber_t = Subscriber<T>;

    /// Publisher type from which this Relay inherits.
    using publisher_t = Publisher<NoData, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const UntypedPublisher* /*publisher*/, const T& /* ignored */) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const UntypedPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const UntypedPublisher* /*publisher*/) override { this->complete(); }
};

// relay identifier ================================================================================================= //

template<class T,                                               // T is a RelayPtr if it:
         class = std::enable_if_t<is_shared_ptr_v<T>>,          // - is a shared_ptr
         class I = typename T::element_type::input_t,           // - has an input type
         class P = typename T::element_type::policy_t,          // - has a policy
         class O = typename T::element_type::output_t>          // - has an output type
struct is_relay : std::is_convertible<T, RelayPtr<I, P, O>> {}; // - can be converted to a RelayPtr

/// constexpr boolean that is true only if T is a RelayPtr
template<class T, class = void>
constexpr bool is_relay_v = false;
template<class T>
constexpr bool is_relay_v<T, decltype(is_relay<T>(), void())> = is_relay<T>::value;

NOTF_CLOSE_NAMESPACE
