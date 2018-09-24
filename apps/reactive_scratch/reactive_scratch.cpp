#include <atomic>

#include "notf/common/vector.hpp"
#include "notf/meta/pointer.hpp"
#include "notf/meta/traits.hpp"
#include "notf/reactive/subscriber.hpp"

template<class T>
struct SingleSubscriber {
    static constexpr bool is_multi_subscriber = false;

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

    bool addSubscriber(notf::SubscriberPtr<T> subscriber)
    {
        if (!notf::weak_ptr_empty(m_subscriber)) {
            NOTF_THROW(notf::logic_error, "Cannot connect multiple Subscribers to a single-subscriber Publisher");
        }
        m_subscriber = std::move(subscriber);
        return true;
    }

    void clear() { m_subscriber.reset(); }

private:
    notf::SubscriberWeakPtr<T> m_subscriber;
};

template<class T>
struct MultiSubscriber {
    static constexpr bool is_multi_subscriber = true;

    /// lambda must take a single argument : notf::Subscriber<T>*
    template<class Lambda>
    void on_each(Lambda&& lambda)
    {
        for (size_t i = 0, end = m_subscribers.size(); i < end;) {
            // call the `on_next` method on all valid subscribers
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

    bool addSubscriber(notf::SubscriberPtr<T> subscriber)
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

    void clear() { m_subscribers.clear(); }

private:
    std::vector<notf::SubscriberWeakPtr<T>> m_subscribers;
};

template<class T, class Subscribers>
class Publisher {

protected:
    /// Internal default "error" operation, accessible from subclasses.
    void _error(const std::exception& error)
    {
        NOTF_ASSERT(!m_is_completed);
        m_is_completed = true;
        m_subscribers.on_each([&error](notf::Subscriber<T>* subscriber) { subscriber->on_error(error); });
        m_subscribers.clear();
    }

    /// Internal default "complete" operation, accessible from subclasses.
    void _complete()
    {
        NOTF_ASSERT(!m_is_completed);
        m_is_completed = true;
        m_subscribers.on_each([](notf::Subscriber<T>* subscriber) { subscriber->on_complete(); });
        m_subscribers.clear();
    }

    /// Call `on_next` on all valid Subscribers.
    void _next(const T& value)
    {
        NOTF_ASSERT(!m_is_completed);
        m_subscribers.on_each([&value](notf::Subscriber<T>* subscriber) { subscriber->on_next(value); });
    }

private:
    /// All conntected Subscribers.
    Subscribers m_subscribers;

    /// Tests whether the Publisher has completed.
    std::atomic_bool m_is_completed = false;
};

int main() { return 0; }
