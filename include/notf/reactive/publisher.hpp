#pragma once

#include <vector>

#include "notf/meta/pointer.hpp"

#include "notf/reactive/fwd.hpp"

NOTF_OPEN_NAMESPACE

// single/multi subscriber policies ================================================================================= //

namespace detail {

/// Policy for a Publisher function with a single Subscriber.
template<class T>
struct SingleSubscriber {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Invoke the given lambda on each Subscriber.
    /// @param lambda   Lambda to invoke, must take a single argument : Subscriber<T>*.
    template<class Lambda>
    void on_each(Lambda&& lambda) {
        if (auto subscriber = m_subscriber.lock(); subscriber) {
            lambda(subscriber.get());
        } else {
            m_subscriber.reset();
        }
    }

    /// Sets a new Subscriber.
    /// @param subscriber   Subscriber to add.
    /// @returns            True, iff the given subscriber is now the only subscriber of the publisher.
    bool add(SubscriberPtr<T> subscriber) {
        if (!m_subscriber.expired()) {
            return false; // there's already a subscriber connected
        }
        m_subscriber = std::move(subscriber);
        return true;
    }

    /// Removes an existing Subscriber.
    void clear() { m_subscriber.reset(); }

    /// Number of connected Subscribers.
    size_t get_subscriber_count() const { return m_subscriber.lock() ? 1 : 0; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The single Subscriber.
    SubscriberWeakPtr<T> m_subscriber;
};

/// Policy for a Publisher function with a multiple Subscribers.
template<class T>
struct MultiSubscriber {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Invoke the given lambda on each Subscriber.
    /// @param lambda   Lambda to invoke, must take a single argument : Subscriber<T>*.
    template<class Lambda>
    void on_each(Lambda&& lambda) {
        for (size_t i = 0, end = m_subscribers.size(); i < end;) {
            // call the given lambda on all valid subscribers
            if (auto subscriber = m_subscribers[i].lock(); subscriber) {
                lambda(subscriber.get());
                ++i;
            }

            // if you come across an expired subscriber, remove it
            else {
                m_subscribers[i] = std::move(m_subscribers.back());
                m_subscribers.pop_back();
                --end;
            }
        }
    }

    /// Adds a new Subscriber.
    /// @param subscriber   Subscriber to add.
    /// @returns            True if `subscriber` was added, `false` if it is already subscribed.
    bool add(SubscriberPtr<T> subscriber) {
        for (size_t i = 0, end = m_subscribers.size(); i < end;) {
            // test if the subscriber is already subscribed
            if (auto existing = m_subscribers[i].lock(); existing) {
                if (subscriber == existing) { return false; }
                ++i;
            }

            // if you come across an expired subscriber, remove it
            else {
                m_subscribers[i] = std::move(m_subscribers.back());
                m_subscribers.pop_back();
                --end;
            }
        }

        // add the new subscriber
        m_subscribers.emplace_back(subscriber);
        return true;
    }

    /// Removes all existing Subscribers.
    void clear() { m_subscribers.clear(); }

    /// Number of connected Subscribers.
    size_t get_subscriber_count() const { return m_subscribers.size(); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All unique Subscribers.
    std::vector<SubscriberWeakPtr<T>> m_subscribers;
};

} // namespace detail

// publisher base =================================================================================================== //

/// Base class for all Publisher functions.
struct AnyPublisher {

    // types ----------------------------------------------------------------------------------- //
public:
    /// State of the Publisher.
    /// The state transition diagram is pretty easy:
    ///
    ///                   +-> FAILED
    ///                 /
    ///   -> RUNNING - + --> COMPLETED
    ///
    enum class State : size_t { // make it a word wide to squelsh warnings about padding
        RUNNING,
        COMPLETED,
        FAILED,
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(AnyPublisher);

    /// Default constructor.
    AnyPublisher() = default;

    /// Virtual destructor.
    virtual ~AnyPublisher() = default;

    /// Called when an untyped Subscriber wants to subscribe to this Publisher.
    /// @param subscriber   New Subscriber.
    /// @returns            True iff the Subscriber was added, false if it was rejected.
    virtual bool subscribe(AnySubscriberPtr /*subscriber*/) = 0;

    /// Number of connected Subscribers.
    virtual size_t get_subscriber_count() const = 0;
};

// typed publisher ================================================================================================== //

namespace detail {

/// Base template for a typed publisher, both data and non-data publishing Publishers derive from it.
template<class T, class Policy>
class TypedPublisher : public AnyPublisher {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type to publish.
    using output_t = T;

    /// Publisher policy type.
    using policy_t = Policy;

private:
    /// Subscriber container as determined by the type.
    using Subscribers = typename Policy::template Subscribers<T>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Checks if this Publisher has already completed (either normally or though an error).
    bool is_completed() const { return m_reactive_state != State::RUNNING; }

    /// Checks if this Publisher has completed with an error.
    bool is_failed() const { return m_reactive_state == State::FAILED; }

    /// Number of connected Subscribers.
    size_t get_subscriber_count() const final { return m_subscribers.get_subscriber_count(); }

    /// Fail method, completes this Publisher as well.
    /// @param exception    The exception that has occurred.
    void error(const std::exception& exception) {
        if (!is_completed()) {
            // call the virtual handler while still running
            _error(exception);

            // transition into the error state
            m_reactive_state = State::FAILED;
            // TODO: is this calling `on_error` twice?
            m_subscribers.on_each([this, &exception](auto* subscriber) { subscriber->on_error(this, exception); });
            m_subscribers.clear();
        }
    }

    /// Complete method, completes the Publisher in a regular fashion.
    void complete() {
        if (!is_completed()) {
            // call the virtual handler while still running
            _complete();

            // transition into the completed state
            m_reactive_state = State::COMPLETED;
            // TODO: is this calling `on_complete` twice?
            m_subscribers.on_each([this](auto* subscriber) { subscriber->on_complete(this); });
            m_subscribers.clear();
        }
    }

    /// @{
    /// Called when a new Subscriber wants to subscribe to this Publisher.
    /// This method has two overloads: one for Subscribers that are correctly typed as (or at least statically
    /// convertible to) SubscriberPtr<T>, and one that is exclusively for untyped Subscribers of type AnySubscriberPtr.
    /// Since types that can be converted to SubscriberPtr<T> can also be converted to AnySubscriberPtr, these methods
    /// would be ambiguous, if it were not for the SFINAE magic in the templated ovleroad.
    /// @param subscriber   New Subscriber.
    /// @returns            True iff the Subscriber was added, false if it was rejected.
    bool subscribe(AnySubscriberPtr subscriber) override // add an untyped subscriber
    {
        auto typed_subscriber = std::dynamic_pointer_cast<Subscriber<T>>(std::move(subscriber));
        if (!typed_subscriber) {
            return false; // type of subscriber does not match this publisher's
        }
        return subscribe(std::move(typed_subscriber));
    }

    template<class Sub, class S = std::decay_t<Sub>>
    std::enable_if_t<
        std::conjunction_v<std::negation<std::is_same<S, AnySubscriberPtr>>, std::is_convertible<S, SubscriberPtr<All>>>,
        bool>
    subscribe(Sub&& subscriber) // add a Subscriber<Everything>
    {
        // cast down to AnySubscriber and up again to Subscriber<T>
        return subscribe(std::static_pointer_cast<Subscriber<T>>(
            std::static_pointer_cast<AnySubscriber>(std::forward<Sub>(subscriber))));
    }

    template<class Sub, class S = std::decay_t<Sub>>
    std::enable_if_t<std::conjunction_v<std::negation<std::is_same<S, AnySubscriberPtr>>, //
                                        std::is_convertible<S, SubscriberPtr<T>>>,
                     bool>
    subscribe(Sub&& subscriber) // add a Subscriber<T>
    {
        // whatever type `Sub` actually is, we need a SubscriberPtr<T>
        auto typed_subscriber = static_cast<SubscriberPtr<T>>(std::forward<Sub>(subscriber));

        // call `on_complete` if the Publisher has already completed (normally or by error, that doesn't matter here)
        if (is_completed()) {
            typed_subscriber->on_complete(this);
            return true; // technically, we've accepted the Subscriber
        }

        // give subclasses a veto
        if (!_subscribe(typed_subscriber)) { return false; }

        // let the subscribers policy handle the newcomer
        return m_subscribers.add(std::move(typed_subscriber));
    }
    /// @}

protected:
    /// Internal error handler, can be implemented by subclasses.
    /// @param exception    The exception that has occurred.
    virtual void _error(const std::exception& exception) {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this, &exception](auto* subscriber) { subscriber->on_error(this, exception); });
    }

    /// Internal completion handler, can be implemented by subclasses.
    virtual void _complete() {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this](auto* subscriber) { subscriber->on_complete(this); });
    }

    /// Called when a new Subscriber is about to be subscribed to this Publisher.
    /// @param subscriber   Subscriber about to be subscribed.
    /// @returns            True to go ahead, false if the Subscriber should not be added to the list of subscribers.
    virtual bool _subscribe(SubscriberPtr<T>& /*subscriber*/) {
        NOTF_ASSERT(!this->is_completed());
        return true;
    }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// All conntected Subscribers.
    Subscribers m_subscribers;

private:
    /// Tests whether the Publisher has completed.
    State m_reactive_state = State::RUNNING;
};

} // namespace detail

// publisher ======================================================================================================== //

/// Default Publisher function that publishes data of a given type T.
template<class T, class Policy>
class Publisher : public detail::TypedPublisher<T, Policy> {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Publish operation.
    /// Propagates the value to all attached Subscribers.
    /// @param value    Value to propagate to all Subscribers.
    void publish(const T& value) {
        if (!this->is_completed()) { _publish(this, value); }
    }

protected:
    /// Internal publish handler, can be overriden by subclasses.
    /// @param publisher    Publisher pushing the value, usually `this`.
    /// @param value        Value to propagate to all Subscribers.
    virtual void _publish(const AnyPublisher* publisher, const T& value) {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([publisher, &value](auto* subscriber) { subscriber->on_next(publisher, value); });
    }
};

/// Specialization for Publisher functions that produce signals only (no data).
template<class Policy>
class Publisher<None, Policy> : public detail::TypedPublisher<None, Policy> {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Publish operation.
    /// Propagates the call to all attached Subscribers.
    void publish() {
        if (!this->is_completed()) { _publish(this); }
    }

protected:
    /// Internal publish handler, can be overriden by subclasses.
    /// @param publisher    Publisher pushing the value, usually `this`.
    virtual void _publish(const AnyPublisher* publisher) {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([publisher](auto* subscriber) { subscriber->on_next(publisher); });
    }
};

// policies ========================================================================================================= //

namespace detail {

struct SinglePublisherPolicy {
    template<class T>
    using Subscribers = ::notf::detail::SingleSubscriber<T>;
};
struct MultiPublisherPolicy {
    template<class T>
    using Subscribers = ::notf::detail::MultiSubscriber<T>;
};
using DefaultPublisherPolicy = SinglePublisherPolicy;

// publisher identifier ============================================================================================= //

struct PublisherIdentifier {
    template<class T>
    static constexpr auto test() {
        if constexpr (std::conjunction_v<is_shared_ptr<T>,                              //
                                         decltype(_has_policy_t<T>(declval<T>())), //
                                         decltype(_has_output_t<T>(declval<T>()))>) {
            using policy_t = typename T::element_type::policy_t;
            using output_t = typename T::element_type::output_t;
            return std::is_convertible<T, PublisherPtr<output_t, policy_t>>{};
        } else {
            return std::false_type{};
        }
    }

private:
    template<class T>
    static constexpr auto _has_policy_t(const T&) -> decltype(typename T::element_type::policy_t{}, std::true_type{});
    template<class>
    static constexpr auto _has_policy_t(...) -> std::false_type;

    template<class T>
    static constexpr auto _has_output_t(const T&) -> decltype(typename T::element_type::output_t{}, std::true_type());
    template<class>
    static constexpr auto _has_output_t(...) -> std::false_type;
};

} // namespace detail

/// Struct derived either from std::true_type or std::false type, depending on whether T is a Publisher or not.
template<class T>
struct is_publisher : decltype(detail::PublisherIdentifier::test<T>()) {};

/// Constexpr boolean that is true only if T is a PublisherPtr.
template<class T>
static constexpr bool is_publisher_v = is_publisher<T>::value;

NOTF_CLOSE_NAMESPACE
