#pragma once

#include "./reactive.hpp"
#include "notf/meta/pointer.hpp"

NOTF_OPEN_NAMESPACE

// subscriber base ================================================================================================== //

/// Base class for all Subscribers.
struct AnySubscriber {

    NOTF_NO_COPY_OR_ASSIGN(AnySubscriber);

    /// Default Constructor.
    AnySubscriber() = default;

    /// Virtual Destructor.
    virtual ~AnySubscriber() = default;

    /// Default implementation of the "error" operation: throws the given exception.
    /// Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
    /// propagate all the way through the callstack, as this could stop the connected Publisher from delivering any more
    /// messages to other Subscribers.
    /// @param exception    The exception that has occurred.
    virtual void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) { throw exception; }

    /// Default implementation of the "complete" operation does nothing.
    virtual void on_complete(const AnyPublisher* /*publisher*/) {}
};

// subscriber ======================================================================================================= //

/// The basic data Subscriber.
template<class T>
class Subscriber : public AnySubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type to receive.
    using input_t = T;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    /// @param value        Published value.
    virtual void on_next(const AnyPublisher* publisher, const input_t& value) = 0;
};

/// Subscriber specialization for Subscribers that do not take any data, just signals.
template<>
class Subscriber<None> : public AnySubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type to receive.
    using input_t = None;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Abstract "next" operation.
    /// @param publisher    The Publisher publishing the value, for identification purposes only.
    virtual void on_next(const AnyPublisher* publisher) = 0;
};

// subscriber identifier ============================================================================================ //

namespace detail {

struct SubscriberIdentifier {
    template<class T>
    static constexpr auto test()
    {
        if constexpr (std::conjunction_v<is_shared_ptr<T>, decltype(_has_input_t<T>(0))>) {
            using input_t = typename T::element_type::input_t;
            return std::is_convertible<T, SubscriberPtr<input_t>>{};
        }
        else {
            return std::false_type{};
        }
    }

private:
    template<class T>
    static constexpr auto _has_input_t(int) -> decltype(typename T::element_type::input_t{}, std::true_type());
    template<class>
    static constexpr auto _has_input_t(...) -> std::false_type;
};

} // namespace detail

/// Struct derived either from std::true_type or std::false type, depending on whether T is a Subscriber or not.
template<class T>
struct is_subscriber : decltype(detail::SubscriberIdentifier::test<T>()) {};

/// Constexpr boolean that is true only if T is a SubscriberPtr.
template<class T>
static constexpr const bool is_subscriber_v = is_subscriber<T>::value;

NOTF_CLOSE_NAMESPACE
