#pragma once

#include "./publisher.hpp"
#include "./subscriber.hpp"

NOTF_OPEN_NAMESPACE

// any operator ===================================================================================================== //

/// Base class for all Operator functions.
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

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<I>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<O, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Subscriber "next" method, forwards to the Producer's "publish" method by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param value        Published value.
    void on_next(const AnyPublisher* /*publisher*/, const I& value) override { this->publish(value); }

    /// Subscriber "error" method, forwards to the Producer's "fail" method by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

// "None" operator specializations ================================================================================== //

/// Specialization for Operator functions that transmit no data.
template<class Policy>
class Operator<None, None, Policy> : public AnyOperator, public Subscriber<None>, public Publisher<None, Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<None>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<None, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_next(const AnyPublisher* /*publisher*/) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

/// Generator base class.
/// Does not provide an `on_next` implementation by default, because that's what the generator is for.
template<class T, class Policy>
class Operator<None, T, Policy> : public AnyOperator, public Subscriber<None>, public Publisher<T, Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<None>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<T, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// In addition to explicit publishing, generators also offer the ability to publish without input.
    void publish() { this->on_next(nullptr); }
    using publisher_t::publish; // do not overwrite the Publisher's method, but add another overload

    /// Abstract "next" method.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    virtual void on_next(const AnyPublisher* publisher) override = 0;

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

/// Specialization for Operators function that connect a data-producing upstream to a "None" downstream.
template<class T, class Policy>
class Operator<T, None, Policy> : public AnyOperator, public Subscriber<T>, public Publisher<None, Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<T>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<None, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param value        Received value (ignored).
    void on_next(const AnyPublisher* /*publisher*/, const T& /* ignored */) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

/// Specialization for Operators function that take any data as input, ignore it and produce another type downstream.
/// Does not provide an `on_next` implementation by default, because that's what this class is for.
template<class T, class Policy>
class Operator<All, T, Policy> : public AnyOperator, public Subscriber<All>, public Publisher<T, Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<All>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<T, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// In addition to explicit publishing, generators also offer the ability to publish without input.
    void publish() { this->on_next(nullptr); }
    using publisher_t::publish; // do not overwrite the Publisher's method, but add another overload

    /// Abstract "next" method.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    virtual void on_next(const AnyPublisher* publisher, ...) override = 0;

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

/// Specialization for Operators function that take any data as input, ignore it and push a dataless signal downstream.
template<class Policy>
class Operator<All, None, Policy>
    : public AnyOperator, public Subscriber<All>, public Publisher<None, Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Subscriber type from which this Operator inherits.
    using subscriber_t = Subscriber<All>;

    /// Publisher type from which this Operator inherits.
    using publisher_t = Publisher<None, Policy>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Abstract "next" method.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_next(const AnyPublisher* /*publisher*/, ...) override { this->publish(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param exception    The exception that has occurred.
    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) override
    {
        this->error(exception);
    }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    void on_complete(const AnyPublisher* /*publisher*/) override { this->complete(); }
};

NOTF_CLOSE_NAMESPACE
