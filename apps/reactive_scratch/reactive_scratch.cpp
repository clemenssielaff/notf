#include <deque>
#include <iostream>

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/relay.hpp"
#include "notf/reactive/subscriber.hpp"

NOTF_OPEN_NAMESPACE
namespace reactive {

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

// console ========================================================================================================== //

auto ConsoleSubscriber()
{
    struct ConsoleSubscriberImpl : public Subscriber<std::string> {

        void on_next(const detail::PublisherBase*, const std::string& value) final { std::cout << value << std::endl; }

        void on_error(const std::exception& error) final { std::cerr << error.what() << std::endl; }

        void on_complete() final { std::cout << "Completed" << std::endl; }
    };
    return std::make_shared<ConsoleSubscriberImpl>();
}

// manual publisher ================================================================================================= //

template<class T, class Policy = detail::DefaultPublisherPolicy>
auto ManualPublisher()
{
    return std::make_shared<Publisher<T, Policy>>();
}

// cached relay ===================================================================================================== //

template<class T, class Policy = detail::DefaultPublisherPolicy>
auto CachedRelay(size_t cache_size = max_value<size_t>())
{
    struct CachedRelayObj : public Relay<T, Policy> {

        CachedRelayObj(size_t cache_size) : Relay<T, Policy>(), m_cache_size(cache_size) {}

        void on_next(const detail::PublisherBase*, const T& value) final
        {
            if (!this->is_completed()) {
                if (m_cache.size() >= m_cache_size) {
                    m_cache.pop_front();
                }
                m_cache.push_back(value);
                this->publisher_t::publish(value);
            }
        }

        bool _subscribe(valid_ptr<SubscriberPtr<T>>& consumer) final
        {
            NOTF_ASSERT(!this->is_completed());
            for (const auto& cached_value : m_cache) {
                consumer->on_next(this, cached_value);
            }
            return true;
        }

    private:
        size_t m_cache_size;
        std::deque<T> m_cache;
    };
    return std::make_shared<CachedRelayObj>(cache_size);
}

// last value relay ================================================================================================= //

template<class T, class Policy = detail::DefaultPublisherPolicy>
auto LastValueRelay()
{
    struct LastValueObj : public Relay<T, Policy> {

        ~LastValueObj() final
        {
            if (!this->is_completed()) {
                this->complete();
            }
        }

        void on_next(const detail::PublisherBase*, const T& value) final
        {
            NOTF_ASSERT(!this->is_completed());
            m_value = value;
        }

        void _complete() final
        {
            if (m_value) {
                this->publish(m_value.value());
            }
        }

    private:
        std::optional<T> m_value;
    };
    return std::make_shared<LastValueObj>();
}

// pipeline operator ================================================================================================ //



template<class T>
auto PipelineRelay()
{
    struct Operator : public Relay<T, detail::SinglePublisherPolicy> {

        void on_next(const detail::PublisherBase*, const T& value) final
        {
            if (m_is_enabled) {
                this->publish(value);
            }
        }

        void setEnabled(const bool is_enabled) { m_is_enabled = is_enabled; }
        void enable() { setEnabled(true); }
        void disable() { setEnabled(false); }

    private:
        bool m_is_enabled = true;
    };

    return std::make_shared<Operator>();
}

} // namespace reactive

NOTF_CLOSE_NAMESPACE

int main()
{
    NOTF_USING_NAMESPACE;

    auto console = reactive::ConsoleSubscriber();
    auto cached = reactive::CachedRelay<std::string>();
    auto manual = reactive::ManualPublisher<std::string, reactive::detail::MultiPublisherPolicy>();
    auto pipeline = reactive::PipelineRelay<std::string>();
    std::shared_ptr<Relay<std::string, reactive::detail::SinglePublisherPolicy>> blub
        = reactive::PipelineRelay<std::string>();

    {
        auto last = reactive::LastValueRelay<std::string>();
        last->subscribe(console);
        manual->subscribe(last);

        manual->subscribe(cached);
        pipeline->subscribe(console);

        manual->publish("hello");
        manual->publish("derbe");
        manual->publish("world");

        cached->subscribe(pipeline);
        manual->publish("indeed");
    }

    return 0;
}
