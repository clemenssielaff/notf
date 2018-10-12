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
    void on_next(const AnyPublisher* /*publisher*/, const O& value) override { this->publish(value); }

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

// operator identifier ============================================================================================== //

template<class T,                                                     // T is a OperatorPtr if it:
         class = std::enable_if_t<is_shared_ptr_v<T>>,                // - is a shared_ptr
         class I = typename T::element_type::input_t,                 // - has an input type
         class P = typename T::element_type::policy_t,                // - has a policy
         class O = typename T::element_type::output_t>                // - has an output type
struct is_operator : std::is_convertible<T, OperatorPtr<I, P, O>> {}; // - can be converted to a OperatorPtr

/// constexpr boolean that is true only if T is a OperatorPtr
template<class T, class = void>
static constexpr const bool is_operator_v = false;
template<class T>
static constexpr const bool is_operator_v<T, decltype(is_operator<T>(), void())> = is_operator<T>::value;

NOTF_CLOSE_NAMESPACE
