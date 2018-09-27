#pragma once

#include "./publisher.hpp"

NOTF_OPEN_NAMESPACE

// relay base template ============================================================================================== //

/// Base Relay template, combining a Subscriber and Producer (potentially of different types) into a single object.
template<class I, class Policy, class O = I>
class Relay : public Subscriber<I>, public Publisher<O, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Relay inherits.
    using subscriber_t = Subscriber<I>;

    /// Publisher type from which this Relay inherits.
    using publisher_t = Publisher<O, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
};

// relay specializations ============================================================================================ //

/// Default specialization for Relays that have the same input and output types.
template<class T, class Policy>
class Relay<T, T, Policy> : public Subscriber<T>, public Publisher<T, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Relay inherits.
    using subscriber_t = Subscriber<T>;

    /// Publisher type from which this Relay inherits.
    using publisher_t = Publisher<T, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const detail::PublisherBase* /*publisher*/, const T& value) override { this->publish(value); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
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
    void on_next(const detail::PublisherBase* /*publisher*/, const T& /* ignored */) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
};

// relay identifier ================================================================================================= //

template<class T,                                                 // T is a RelayPtr if it:
         class = std::enable_if_t<is_shared_ptr_v<T>>,            // - is a shared_ptr
         class I = typename T::element_type::input_t,             // - has an input type
         class P = typename T::element_type::policy_t,            // - has a policy
         class O = typename T::element_type::output_t>            // - has an output type
struct is_relay_t : std::is_convertible<T, RelayPtr<I, P, O>> {}; // - can be converted to a RelayPtr

/// constexpr boolean that is true only if T is a RelayPtr
template<typename T, typename = void>
constexpr bool is_relay_v = false;
template<typename T>
constexpr bool is_relay_v<T, decltype(is_relay_t<T>(), void())> = is_relay_t<T>::value;

NOTF_CLOSE_NAMESPACE
