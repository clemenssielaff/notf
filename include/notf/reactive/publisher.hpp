#pragma once

#include <atomic>

#include "../common/mutex.hpp"
#include "../common/vector.hpp"
#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// publisher base =================================================================================================== //

namespace detail {

/// Publisher base template, both data and non-data publishing Publishers derive from it.
template<class T>
struct PublisherBase {

    // Befriend subclass so it has access to `m_subscribers`.
    friend struct Publisher<T>;

    /// Type to publish.
    using output_t = T;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default constructor.
    PublisherBase() = default;

    /// Virtual destructor.
    virtual ~PublisherBase() = default;

    /// Checks if this Publisher has already completed.
    /// Due to uncertainty in a multithreaded environment, it is generally not safe to assume that the Publisher is
    /// still running if this method returns `false`, as it might have completed in between the check and the time that
    /// the produced boolean is evaluated.
    /// However, if this method returns `true`, you can be certain that this Publisher has finished.
    bool is_completed() const { return m_is_completed; }

    /// Fail operation, completes this Publisher.
    /// @param exception    The exception that has occurred.
    virtual void error(const std::exception& exception)
    {
        NOTF_GUARD_IF(!is_completed(), std::lock_guard(m_mutex)) { _error(exception); }
    }

    /// Complete operation.
    virtual void complete()
    {
        NOTF_GUARD_IF(!is_completed(), std::lock_guard(m_mutex)) { _complete(); }
    }

    bool subscribe(SubscriberPtr<T> subscriber)
    {
        if (!subscriber) {
            return false;
        }
        NOTF_GUARD_IF(!is_completed(), std::lock_guard(m_mutex))
        {
            if (std::find(m_subscribers.begin(), m_subscribers.end(), subscriber) != m_subscribers.end()) {
                return false; // already subscribed
            }
            if (this->_on_subscribe(subscriber)) {
                m_subscribers.emplace_back(std::move(subscriber));
                return true;
            }
            return false;
        }

        // the Publisher has already completed
        subscriber->on_complete();
        return true;
    }

    detail::PipelineBasePtr<T> get_subscription(const SubscriberPtr<T>& /*subscriber*/) const
    {
        return {}; // TODO: store subscriptions for re-use
    }

    /// Removes an existing Subscriber, if it is subscribed.
    void unsubscribe(const SubscriberPtr<T>& subscriber)
    {
        if (subscriber) {
            NOTF_GUARD_IF(!m_is_completed, std::lock_guard(m_mutex))
            {
                if (auto it = std::find(m_subscribers.begin(), m_subscribers.end(), subscriber);
                    it != m_subscribers.end()) {
                    m_subscribers.erase(it);
                }
            }
        }
    }

protected:
    /// Internal default "error" operation, accessible from subclasses.
    void _error(const std::exception& error)
    {
        NOTF_ASSERT(!m_is_completed && m_mutex.is_locked_by_this_thread());
        m_is_completed = true;
        for (const SubscriberPtr<T>& subscriber : m_subscribers) {
            if (subscriber && !subscriber.unique()) {
                subscriber->on_error(error);
            }
        }
        m_subscribers.clear();
    }

    /// Internal default "complete" operation, accessible from subclasses.
    void _complete()
    {
        NOTF_ASSERT(!m_is_completed && m_mutex.is_locked_by_this_thread());
        m_is_completed = true;
        for (const SubscriberPtr<T>& subscriber : m_subscribers) {
            if (subscriber && !subscriber.unique()) {
                subscriber->on_complete();
            }
        }
        m_subscribers.clear();
    }

    /// Called when a new Subscriber is about to be subscribed to this Publisher.
    /// @param subscriber   Subscriber about to be subscribed.
    /// @returns            True to go ahead, false if the Subscriber should not be added to the list of subscribers.
    virtual bool _on_subscribe(SubscriberPtr<T>& /*subscriber*/)
    {
        NOTF_ASSERT(!m_is_completed && m_mutex.is_locked_by_this_thread());
        return true;
    }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Mutex guarding this Publisher, accessible from subclasses.
    mutable Mutex m_mutex;

private:
    /// All conntected Subscribers.
    std::vector<SubscriberPtr<T>> m_subscribers;

    /// Tests whether the Publisher has completed.
    std::atomic_bool m_is_completed = false;
};

} // namespace detail

// publisher ======================================================================================================== //

/// Default Publisher that publishes data of a given type T.
template<class T>
struct Publisher : public detail::PublisherBase<T> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Publish operation.
    /// Takes the value and propagates it to all attached Subscribers, while removing disconnected Subscriber.
    /// @param value    Value to propagate to all Subscribers.
    virtual void next(const T& value)
    {
        NOTF_GUARD_IF(!this->m_is_completed, std::lock_guard(this->m_mutex)) { _next(value); }
    }

protected:
    /// Internal default "next" operation, accessible from subclasses.
    void _next(const T& value)
    {
        NOTF_ASSERT(!this->m_is_completed && this->m_mutex.is_locked_by_this_thread());
        iterate_and_clean(this->m_subscribers,                                               //
                          [](const auto& subscriber) { return subscriber.use_count() > 1; }, // = validation
                          [&](const auto& subscriber) { subscriber->on_next(value); }        // = action
        );
    }
};

/// Specialization for Publisher that produce signals only (no data).
template<>
struct Publisher<NoData> : public detail::PublisherBase<NoData> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Publish operation.
    /// Propagates the call to all attached Subscribers, while removing disconnected Subscribers.
    virtual void next()
    {
        NOTF_GUARD_IF(!this->m_is_completed, std::lock_guard(this->m_mutex)) { _next(); }
    }

protected:
    /// Internal default "next" operation, accessible from subclasses.
    void _next()
    {
        NOTF_ASSERT(!this->m_is_completed && this->m_mutex.is_locked_by_this_thread());
        iterate_and_clean(this->m_subscribers,                                               //
                          [](const auto& subscriber) { return subscriber.use_count() > 1; }, // = validation
                          [&](const auto& subscriber) { subscriber->on_next(); }             // = action
        );
    }
};

NOTF_CLOSE_NAMESPACE
