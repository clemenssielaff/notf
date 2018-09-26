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

NOTF_CLOSE_NAMESPACE
