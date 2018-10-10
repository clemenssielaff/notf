#include <deque>
#include <iostream>
#include <optional>

#include "notf/reactive/pipeline.hpp"
#include "notf/reactive/publisher.hpp"
#include "notf/reactive/relay.hpp"
#include "notf/reactive/subscriber.hpp"

NOTF_OPEN_NAMESPACE
namespace reactive {

// console ========================================================================================================== //

auto ConsoleSubscriber()
{
    struct ConsoleSubscriberImpl : public Subscriber<std::string> {

        void on_next(const UntypedPublisher*, const std::string& value) final { std::cout << value << std::endl; }

        void on_error(const UntypedPublisher*, const std::exception& error) final
        {
            std::cerr << error.what() << std::endl;
        }

        void on_complete(const UntypedPublisher*) final
        { /*std::cout << "Completed" << std::endl;*/
        }
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
    struct CachedRelayObj : public Relay<T, T, Policy> {

        CachedRelayObj(size_t cache_size) : Relay<T, T, Policy>(), m_cache_size(cache_size) {}

        void on_next(const UntypedPublisher*, const T& value) final
        {
            if (!this->is_completed()) {
                if (m_cache.size() >= m_cache_size) {
                    m_cache.pop_front();
                }
                m_cache.push_back(value);
                Relay<T, T, Policy>::publisher_t::publish(value);
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
    struct LastValueObj : public Relay<T, T, Policy> {

        ~LastValueObj() final
        {
            if (!this->is_completed()) {
                this->complete();
            }
        }

        void on_next(const UntypedPublisher*, const T& value) final
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace reactive

NOTF_CLOSE_NAMESPACE
NOTF_USING_NAMESPACE;

void test1()
{

    auto console = reactive::ConsoleSubscriber();
    auto cached = reactive::CachedRelay<std::string>();
    auto manual = reactive::ManualPublisher<std::string, detail::MultiPublisherPolicy>();
    auto pipeline = std::make_shared<detail::TogglePipelineOperator<std::string>>();

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
}

void test2()
{
    auto manual = reactive::ManualPublisher<std::string>();
    auto console = reactive::ConsoleSubscriber();

    manual->publish("noshow");
    {
        // test if the pipeline can temporarely connect a publisher to a subscriber
        auto pipeline = manual | console;
        manual->publish("1");
    }
    {
        auto pipeline = manual | console;
        manual->publish("2");
    }
    {
        // pipeline ending in an r-value subscriber
        // & enabling / disabling of pipeline
        auto pipeline = manual | reactive::ConsoleSubscriber();
        manual->publish("3");
        pipeline.disable();
        manual->publish("noshow");
        pipeline.enable();
        manual->publish("4");
    }
    {
        // test with one r-value intermediary
        auto pipeline = manual | reactive::CachedRelay<std::string>() | console;
        manual->publish("5");
    }
    {
        // test with two r-value intermediary
        auto pipeline = manual | reactive::CachedRelay<std::string>() | reactive::CachedRelay<std::string>() | console;
        manual->publish("6");
    }
    manual->publish("noshow");
}

int main()
{
    test2();

    return 0;
}
