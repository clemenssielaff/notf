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

        void on_complete() final
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

class DisableableMixin {

public:
    virtual ~DisableableMixin() = default;

    void setEnabled(const bool is_enabled) { m_is_enabled = is_enabled; }
    void enable() { setEnabled(true); }
    void disable() { setEnabled(false); }

protected:
    bool m_is_enabled = true;
    bool padding[7];
};

class PipelineBase {
public:
    using Toggle = std::shared_ptr<DisableableMixin>;

    using Operators = std::vector<std::shared_ptr<::notf::detail::PublisherBase>>;

protected:
    PipelineBase(Toggle&& toggle, Operators&& operators = {})
        : m_toggle(std::move(toggle)), m_operators(std::move(operators))
    {}

public:
    void setEnabled(const bool is_enabled) { m_toggle->setEnabled(is_enabled); }
    void enable() { setEnabled(true); }
    void disable() { setEnabled(false); }

    void clear()
    {
        m_toggle.reset();
        m_operators.clear();
    }

    // private:
public:
    Toggle m_toggle;
    Operators m_operators;
};

} // namespace detail

template<class T>
auto PipelineRelay()
{
    struct PipelineRelayOperator : public Relay<T, detail::SinglePublisherPolicy>, detail::DisableableMixin {

        void on_next(const detail::PublisherBase*, const T& value) final
        {
            if (m_is_enabled) {
                this->publish(value);
            }
        }
    };
    return std::make_shared<PipelineRelayOperator>();
}

template<class LastOperator>
class Pipeline : public detail::PipelineBase {
public:
    using typename detail::PipelineBase::Operators;
    using typename detail::PipelineBase::Toggle;

    using last_t = LastOperator;

public:
    Pipeline(Toggle&& toggle, LastOperator&& last = {}, Operators&& operators = {})
        : detail::PipelineBase(std::move(toggle), std::move(operators)), m_last(std::move(last))
    {}

    // private:
public:
    LastOperator m_last;
};

template<class T>
struct is_pipeline : std::false_type {};
template<class T>
struct is_pipeline<Pipeline<T>> : std::true_type {};
template<class T>
constexpr bool is_pipeline_v = is_pipeline<T>::value;

/// Metafunction that is only defined if P is a Publisher/Pipeline type that can be connected to Subscriber type S.
template<class P, class S, class = void>
struct is_operator_compatible : std::false_type {};

template<class P, class S>
struct is_operator_compatible<P, S, std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>)>>
    : std::is_same<typename P::element_type::output_t, typename S::element_type::input_t> {};

template<class P, class S>
struct is_operator_compatible<P, S, std::enable_if_t<all(is_pipeline_v<P>, is_subscriber_v<S>)>>
    : std::is_same<typename P::last_t::element_type::output_t, typename S::element_type::input_t> {};

/// consexpr boolean that is true iff P is a Publisher type that can be connected to Subscriber type S.
template<class P, class S, class = void>
constexpr bool is_compatible_v = false;
template<class P, class S>
constexpr bool is_compatible_v<P, S, decltype(is_operator_compatible<P, S>(), void())> = true;

} // namespace reactive

/// Connect an l-value Publisher to an l-value Subscriber
template<class P, class S, class = std::enable_if_t<all(is_publisher_v<P>, reactive::is_compatible_v<P, S>)>>
auto operator|(P& publisher, S& subscriber)
{
    using output_t = typename P::element_type::output_t;
    auto relay = reactive::PipelineRelay<output_t>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return reactive::Pipeline<decltype(relay)>(std::move(relay));
}

/// Connect an l-value Publisher to an r-value Subscriber
template<class P, class S,
         class = std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, reactive::is_compatible_v<P, S>)>>
auto operator|(P& publisher, S&& subscriber)
{
    using output_t = typename P::element_type::output_t;
    auto relay = reactive::PipelineRelay<output_t>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return reactive::Pipeline<S>(std::move(relay), std::move(subscriber));
}

///// Connect an r-value Publisher to an l-value Subscriber
//template<class P, class S, class = std::enable_if_t<all(is_publisher_v<P>, reactive::is_compatible_v<P, S>)>>
//auto operator|(P&& publisher, S& subscriber)
//{
//    using output_t = typename P::element_type::output_t;
//    auto relay = reactive::PipelineRelay<output_t>();
//    publisher->subscribe(relay);
//    relay->subscribe(subscriber);
//    return reactive::Pipeline<decltype(relay)>(std::move(relay));
//}

///// Connect an r-value Publisher to an r-value Subscriber
//template<class P, class S,
//         class = std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, reactive::is_compatible_v<P, S>)>>
//auto operator|(P&& publisher, S&& subscriber)
//{
//    using output_t = typename P::element_type::output_t;
//    auto relay = reactive::PipelineRelay<output_t>();
//    publisher->subscribe(relay);
//    relay->subscribe(subscriber);
//    return reactive::Pipeline<S>(std::move(relay), std::move(subscriber));
//}

/// Connect an r-value Pipeline to an l-value Subscriber
template<class P, class S,
         class = std::enable_if_t<all(reactive::is_pipeline_v<P>, is_subscriber_v<S>, reactive::is_compatible_v<P, S>)>>
auto operator|(P&& pipeline, S& subscriber)
{
    pipeline.m_last->subscribe(subscriber);
    return pipeline;
}

/// Connect an r-value Pipeline to an r-value Subscriber
template<class P, class S,
         class = std::enable_if_t<all(reactive::is_pipeline_v<P>, is_subscriber_v<S>, reactive::is_compatible_v<P, S>)>>
auto operator|(P&& pipeline, S&& subscriber)
{
    pipeline.m_last->subscribe(subscriber);
    pipeline.m_operators.emplace_back(std::move(pipeline.m_last));
    return reactive::Pipeline<S>(std::move(pipeline.m_toggle), std::move(subscriber), std::move(pipeline.m_operators));
}

NOTF_CLOSE_NAMESPACE NOTF_USING_NAMESPACE;

void test1()
{

    auto console = reactive::ConsoleSubscriber();
    auto cached = reactive::CachedRelay<std::string>();
    auto manual = reactive::ManualPublisher<std::string, reactive::detail::MultiPublisherPolicy>();
    auto pipeline = reactive::PipelineRelay<std::string>();

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
        // the pipeline can be manually reset
        auto pipeline = manual | console;
        manual->publish("2");
        pipeline.clear();
        manual->publish("noshow"); // noshow
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
