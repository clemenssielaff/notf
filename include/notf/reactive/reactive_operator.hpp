#pragma once

#include "./publisher.hpp"
#include "./subscriber.hpp"

NOTF_OPEN_NAMESPACE

// any operator ===================================================================================================== //

/// Base class for all Operators.
struct AnyOperator {

    NOTF_NO_COPY_OR_ASSIGN(AnyOperator);

    /// Default constructor.
    AnyOperator() = default;

    /// Virtual destructor.
    virtual ~AnyOperator() = default;
};

// operator<I, O> template ========================================================================================== //

/// Base Operator template, combining a Subscriber and Producer (potentially of different types) into a single object.
template<class I, class O = I, class Policy = detail::DefaultPublisherPolicy>
class Operator : public AnyOperator, public Subscriber<I>, public Publisher<O, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<I>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<O, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const AnyPublisher* /*publisher*/, const I& value) override { this->publish(value); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

// "None" operator specializations ================================================================================== //

/// Specialization for Operators that transmit no data.
template<class Policy>
class Operator<None, None, Policy> : public AnyOperator, public Subscriber<None>, public Publisher<None, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<None>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<None, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const AnyPublisher* /*publisher*/) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

/// Specialization for Operators that connect a data-producing upstream to a "None" downstream.
template<class T, class Policy>
class Operator<T, None, Policy> : public AnyOperator, public Subscriber<T>, public Publisher<None, Policy> {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<T>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<None, Policy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const AnyPublisher* /*publisher*/, const T& /* ignored */) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

NOTF_CLOSE_NAMESPACE
