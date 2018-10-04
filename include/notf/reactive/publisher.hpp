#pragma once

#include <vector>

#include "../meta/pointer.hpp"
#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// single/multi subscriber policies ================================================================================= //

namespace detail {

/// Policy for a Publisher with a single Subscriber.
template<class T>
struct SingleSubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Policy identifier.
    static constexpr bool is_multi_subscriber = false;

    /// Invoke the given lambda on each Subscriber.
    /// @param lambda   Lambda to invoke, must take a single argument : Subscriber<T>*.
    template<class Lambda>
    void on_each(Lambda&& lambda)
    {
        if (auto subscriber = m_subscriber.lock(); subscriber) {
            lambda(subscriber.get());
        }
        else {
            m_subscriber.reset();
        }
    }

    /// Sets a new Subscriber.
    /// @throws notf::logic_error   If the Publisher already has a Subscriber.
    /// @returns                    Always true (if the function didn't throw).
    bool add(SubscriberPtr<T> subscriber)
    {
        if (!m_subscriber.expired()) {
            NOTF_THROW(logic_error, "Cannot connect multiple Subscribers to a single-subscriber Publisher");
        }
        m_subscriber = std::move(subscriber);
        return true;
    }

    /// Removes an existing Subscriber.
    void clear() { m_subscriber.reset(); }

    /// Number of connected Subscribers.
    size_t get_subscriber_count() const { return m_subscriber.lock() ? 1 : 0; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The single Subscriber.
    SubscriberWeakPtr<T> m_subscriber;
};

/// Policy for a Publisher with a multiple Subscribers.
template<class T>
struct MultiSubscriber {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Policy identifier.
    static constexpr bool is_multi_subscriber = true;

    /// Invoke the given lambda on each Subscriber.
    /// @param lambda   Lambda to invoke, must take a single argument : Subscriber<T>*.
    template<class Lambda>
    void on_each(Lambda&& lambda)
    {
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
    bool add(SubscriberPtr<T> subscriber)
    {
        for (size_t i = 0, end = m_subscribers.size(); i < end;) {
            // test if the subscriber is already subscribed
            if (auto existing = m_subscribers[i].lock(); existing) {
                if (subscriber == existing) {
                    return false;
                }
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

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All unique Subscribers.
    std::vector<SubscriberWeakPtr<T>> m_subscribers;
};

// publisher base =================================================================================================== //

/// Base class for all Publishers.
/// Is only used for identification purposes.
struct PublisherBase {

    NOTF_NO_COPY_OR_ASSIGN(PublisherBase);

    /// Default constructor.
    PublisherBase() = default;

    /// Virtual destructor.
    virtual ~PublisherBase() = default;
};

// typed publisher ================================================================================================== //

/// Base template for a Type, both data and non-data publishing Publishers derive from it.
template<class T, class Policy>
class TypedPublisher : public PublisherBase {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type to publish.
    using output_t = T;

    /// Publisher policy type.
    using policy_t = Policy;

    /// Whether or not this Publisher allows multiple Subscribers or not.
    static constexpr bool is_multi_subscriber = Policy::is_multi_subscriber;

    /// State of the Publisher.
    /// The state transition diagram is pretty easy:
    ///
    ///   -> RUNNING - + --> COMPLETED
    ///                 \
    ///                  +-> FAILED
    ///
    enum class State : size_t { // make it a word wide to squelsh warnings about padding
        RUNNING,
        COMPLETED,
        FAILED,
    };

private:
    /// Subscriber container as determined by the type.
    using Subscribers = typename Policy::template Subscribers<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Checks if this Publisher has already completed (either normally or though an error).
    bool is_completed() const { return m_state != State::RUNNING; }

    /// Checks if this Publisher has completed with an error.
    bool is_failed() const { return m_state == State::FAILED; }

    /// Number of connected Subscribers.
    size_t get_subscriber_count() const { return m_subscribers.get_subscriber_count(); }

    /// Fail operation, completes this Publisher.
    /// @param exception    The exception that has occurred.
    void error(const std::exception& exception)
    {
        if (!is_completed()) {
            // call the virtual error handler while still running
            _error(exception);

            // transition into the error state
            m_state = State::FAILED;
            m_subscribers.on_each([this, &exception](auto* subscriber) { subscriber->on_error(this, exception); });
            m_subscribers.clear();
        }
    }

    /// Complete operation, completes the Publisher in a regular fashion.
    void complete()
    {
        if (!is_completed()) {
            // call the virtual error handler while still running
            _complete();

            // transition into the completed state
            m_state = State::COMPLETED;
            m_subscribers.on_each([this](auto* subscriber) { subscriber->on_complete(this); });
            m_subscribers.clear();
        }
    }

    /// Called when a new Subscriber wants to subscribe to this Publisher.
    /// @param subscriber   New Subscriber.
    /// @returns            True iff the Subscriber was added, false if it was rejected.
    /// @throws notf:logic_error    If the Publisher can only have a single Subscriber.
    bool subscribe(valid_ptr<SubscriberPtr<T>> subscriber)
    {
        // call complete if the Publisher has already completed (even if through an error, that doesn't matter here)
        if (is_completed()) {
            subscriber->on_complete(this);
            return true; // technically, we've accepted the Subscriber
        }

        // give subclasses a veto
        if (!_subscribe(subscriber)) {
            return false;
        }

        // let the Subscriber policy handle the newcomer
        return m_subscribers.add(std::move(subscriber));
    }

protected:
    /// Internal error handler, can be implemented by subclasses.
    /// @param exception    The exception that has occurred.
    virtual void _error(const std::exception& exception)
    {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this, &exception](auto* subscriber) { subscriber->on_error(this, exception); });
    }

    /// Internal completion handler, can be implemented by subclasses.
    virtual void _complete()
    {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this](auto* subscriber) { subscriber->on_complete(this); });
    }

    /// Called when a new Subscriber is about to be subscribed to this Publisher.
    /// @param subscriber   Subscriber about to be subscribed.
    /// @returns            True to go ahead, false if the Subscriber should not be added to the list of subscribers.
    virtual bool _subscribe(valid_ptr<SubscriberPtr<T>>& /*subscriber*/)
    {
        NOTF_ASSERT(!this->is_completed());
        return true;
    }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// All conntected Subscribers.
    Subscribers m_subscribers;

    /// Tests whether the Publisher has completed.
    State m_state;
};

} // namespace detail

// publisher ======================================================================================================== //

/// Default Publisher that publishes data of a given type T.
template<class T, class Policy>
class Publisher : public detail::TypedPublisher<T, Policy> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Publish operation.
    /// Propagates the value to all attached Subscribers.
    /// @param value    Value to propagate to all Subscribers.
    void publish(const T& value)
    {
        if (!this->is_completed()) {
            _publish(value);
        }
    }

protected:
    /// Internal publish handler, can be overriden by subclasses.
    /// @param value    Value to propagate to all Subscribers.
    virtual void _publish(const T& value)
    {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this, &value](auto* subscriber) { subscriber->on_next(this, value); });
    }
};

/// Specialization for Publisher that produce signals only (no data).
template<class Policy>
class Publisher<NoData, Policy> : public detail::TypedPublisher<NoData, Policy> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Publish operation.
    /// Propagates the call to all attached Subscribers.
    void publish()
    {
        if (!this->is_completed()) {
            _publish();
        }
    }

protected:
    /// Internal publish handler, can be overriden by subclasses.
    void _publish()
    {
        NOTF_ASSERT(!this->is_completed());
        this->m_subscribers.on_each([this](auto* subscriber) { subscriber->on_next(this); });
    }
};

// policies ========================================================================================================= //

namespace detail {

using ::notf::detail::PublisherBase;

struct SinglePublisherPolicy {
    template<class T>
    using Subscribers = ::notf::detail::SingleSubscriber<T>;
};
struct MultiPublisherPolicy {
    template<class T>
    using Subscribers = ::notf::detail::MultiSubscriber<T>;
};
using DefaultPublisherPolicy = SinglePublisherPolicy;

} // namespace detail

// publisher identifier ============================================================================================= //

template<class T,                                                    // T is a PublisherPtr if it:
         class = std::enable_if_t<is_shared_ptr_v<T>>,               // - is a shared_ptr
         class P = typename T::element_type::policy_t,               // - has a policy
         class O = typename T::element_type::output_t>               // - has an output type
struct is_publisher : std::is_convertible<T, PublisherPtr<O, P>> {}; // - can be converted to a PublisherPtr

/// constexpr boolean that is true only if T is a PublisherPtr
template<class T, class = void>
constexpr bool is_publisher_v = false;
template<class T>
constexpr bool is_publisher_v<T, decltype(is_publisher<T>(), void())> = is_publisher<T>::value;

NOTF_CLOSE_NAMESPACE
