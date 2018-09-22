#include <deque>
#include <iostream>
#include <optional>
#include <string>

#include "notf/meta/numeric.hpp"

#include "notf/common/timer_pool.hpp"
#include "notf/common/vector.hpp"

#include "notf/reactive/pipeline.hpp"
#include "notf/reactive/publisher.hpp"
#include "notf/reactive/relay.hpp"
#include "notf/reactive/subscriber.hpp"

NOTF_USING_NAMESPACE;

// ------------------------------------------------------------------------------------------------------------------ //

namespace publishers {

auto ConsoleSubscriber()
{
    struct ConsoleSubscriberImpl : public Subscriber<std::string> {
        void on_next(const std::string& value) override { std::cout << value << std::endl; }
        void on_error(const std::exception& error) override { std::cerr << error.what() << std::endl; }
        void on_complete() override {}
    };
    return std::make_shared<ConsoleSubscriberImpl>();
}

// ------------------------------------

template<class T>
auto ManualPublisher()
{
    return std::make_shared<Publisher<T>>();
}

// ------------------------------------

template<class T>
auto CachedRelay(size_t cache_size = max_value<size_t>())
{
    struct CachedRelayObj : public Relay<T> {

        CachedRelayObj(size_t cache_size) : Relay<T>(), m_cache_size(cache_size) {}

        ~CachedRelayObj() override = default;

        void next(const T& value) override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
            {
                if (m_cache.size() >= m_cache_size) {
                    m_cache.pop_front();
                }
                m_cache.push_back(value);
                Publisher<T>::_next(value);
            }
        }

        bool _on_subscribe(SubscriberPtr<T>& consumer) override
        {
            NOTF_ASSERT(!this->is_completed() && this->m_mutex.is_locked_by_this_thread());
            for (const auto& cached_value : m_cache) {
                consumer->on_next(cached_value);
            }
            return true;
        }

    private:
        std::deque<T> m_cache;
        size_t m_cache_size;
    };
    return std::make_shared<CachedRelayObj>(cache_size);
}

//// ------------------------------------

template<class T>
auto LastValueRelay()
{
    struct LastValueObj : public Relay<T> {
        void next(const T& value) override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex)) { m_value = value; }
        }
        void complete() override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
            {
                if (m_value) {
                    this->_next(m_value.value());
                }
                this->_complete();
            }
        }

    private:
        std::optional<T> m_value;
    };
    return std::make_shared<LastValueObj>();
}

// ------------------------------------

template<class T>
auto Return(T&& value)
{
    struct ReturnObj : public Publisher<T> {
        ReturnObj(T&& value) : Publisher<T>(), m_value(std::forward<T>(value)) {}

        bool _on_subscribe(SubscriberPtr<T>& consumer) override
        {
            NOTF_ASSERT(!this->is_completed() && this->m_mutex.is_locked_by_this_thread());
            consumer->on_next(m_value);
            consumer->on_complete();
            return true;
        }

    private:
        const T m_value;
    };
    return std::make_shared<ReturnObj>(std::forward<T>(value));
}

// ------------------------------------

// template<class T>
// auto Empty()
//{
//    struct EmptyPublisherImpl : public Publisher<T> {
//        Pipeline<T> create_pipeline(PublisherPtr<T>, SubscriberPtr<T> consumer) override
//        {
//            consumer->on_complete();
//            return {};
//        }
//    };
//    return std::make_shared<EmptyPublisherImpl>(std::make_unique<EmptyPublisherImpl>());
//}

//// ------------------------------------

// template<class T>
// auto Never()
//{
//    struct NeverPublisherImpl : public Publisher<T> {
//        Pipeline<T> create_pipeline(PublisherPtr<T>, SubscriberPtr<T>) override { return {}; }
//    };
//    return std::make_shared<NeverPublisherImpl>(std::make_unique<NeverPublisherImpl>());
//}

//// ------------------------------------

// template<class T>
// auto Throw(std::exception exception)
//{
//    struct ThrowPublisherImpl : public Publisher<T> {
//        ThrowPublisherImpl(std::exception&& exception) : Publisher<T>(), m_exception(std::move(exception)) {}
//        Pipeline<T> create_pipeline(PublisherPtr<T>, SubscriberPtr<T> consumer) override
//        {
//            consumer->on_error(m_exception);
//            return {};
//        }

//    private:
//        const std::exception m_exception;
//    };
//    return std::make_shared<ThrowPublisherImpl>(std::make_unique<ThrowPublisherImpl>(std::move(exception)));
//}

// ------------------------------------

/// End is exclusive
template<class T>
auto Counter(T start, duration_t interval, T end = max_value<T>(), T step = 1)
{
    struct CounterObj : public Publisher<T> {
        CounterObj(T start, T step, T end, duration_t interval) : Publisher<T>(), m_counter(start)
        {
            m_timer = IntervalTimer::create(
                [&, step, end] {
                    NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
                    {
                        m_counter += step;
                        if (m_counter == end) {
                            this->_complete();
                        }
                        else {
                            this->_next(m_counter);
                        }
                    }
                },
                interval);
        }

        ~CounterObj() override
        {
            NOTF_GUARD(std::lock_guard(this->m_mutex));
            if (m_timer) {
                m_timer->stop();
                m_timer.reset();
            }
        }

        void complete() override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
            {
                m_timer->stop();
                this->_complete();
                m_timer.reset();
            }
        }

        bool _on_subscribe(SubscriberPtr<T>&) override
        {
            NOTF_ASSERT(!this->is_completed() && this->m_mutex.is_locked_by_this_thread());
            m_timer->start();
            return true;
        }

    private:
        T m_counter;
        IntervalTimerPtr m_timer;
    };
    return std::make_shared<CounterObj>(start, step, end, std::move(interval));
}

// ------------------------------------

template<class source_t, class target_t>
auto Adapter()
{
    struct AdapterObj : public Relay<source_t, target_t> {
        using input_t = source_t;
        using output_t = target_t;
        AdapterObj() : Relay<source_t, target_t>() {}
        void on_next(const source_t& value) override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
            {
                if constexpr (std::is_same_v<source_t, target_t>) {
                    this->_next(value);
                }
                else if constexpr (std::is_same_v<target_t, std::string>) {
                    this->_next(fmt::to_string(value));
                }
                else if constexpr (std::is_same_v<target_t, NoData>) {
                    this->_next();
                }
            }
        }
    };
    return std::make_shared<AdapterObj>();
}

// ------------------------------------

template<class state_t, class result_t, class Condition, class Iterate, class Refine>
auto Generator(state_t&& initial_state, Iterate&& iterate, Condition&& continue_condition, Refine&& refine)
{
    struct GeneratorRelay : public Relay<NoData, result_t> {
        GeneratorRelay(state_t&& state, Iterate&& iterate, Condition&& continue_condition, Refine&& refine)
            : Relay<NoData, result_t>()
            , m_continue_condition(std::move(continue_condition))
            , m_iterate(std::move(iterate))
            , m_refine(std::move(refine))
            , m_state(std::move(state))
        {}
        void on_next() override
        {
            NOTF_GUARD_IF(!this->is_completed(), std::lock_guard(this->m_mutex))
            {
                try {
                    this->_next(m_refine(m_state));
                    m_iterate(m_state);
                    if (!m_continue_condition(m_state)) {
                        this->_complete();
                    }
                }
                catch (const std::exception& exception) {
                    this->_error(exception);
                }
            }
        }

    private:
        Condition m_continue_condition;
        Iterate m_iterate;
        Refine m_refine;
        state_t m_state;
    };

    return std::make_shared<GeneratorRelay>(std::forward<state_t>(initial_state), std::forward<Iterate>(iterate),
                                            std::forward<Condition>(continue_condition), std::forward<Refine>(refine));
}

template<class state_t, class Iterate>
auto Generator(state_t&& initial_state, Iterate&& iterate)
{
    return Generator<state_t, state_t>(std::forward<state_t>(initial_state), // initial state
                                       std::forward<Iterate>(iterate),       // iterate
                                       [](const state_t&) { return true; },  // continue forever
                                       [](const state_t& s) { return s; });  // refinement is trivial
}

template<class state_t, class Iterate, class Condition>
auto Generator(state_t&& initial_state, Iterate&& iterate, Condition&& continue_condition)
{
    return Generator<state_t, state_t>(std::forward<state_t>(initial_state),        // initial state
                                       std::forward<Iterate>(iterate),              // iterate
                                       std::forward<Condition>(continue_condition), // continue_condition
                                       [](const state_t& s) { return s; });         // refinement is trivial
}

} // namespace publishers

// ================================================================================================================== //

void part1()
{
    using namespace publishers;

    auto console = ConsoleSubscriber();
    {
        auto async = LastValueRelay<std::string>();
        NOTF_GUARD(async | console);
        {
            auto writer = ManualPublisher<std::string>();
            auto replay = CachedRelay<std::string>(2);

            NOTF_GUARD(writer | async);
            NOTF_GUARD(writer | replay);

            writer->next("1");
            writer->next("2");
            writer->next("3");

            NOTF_GUARD(replay | console);
            writer->next("4");
            writer->next("5");
            writer->next("6");
            writer->next("7");
            writer->complete();
            writer->next("8");
            writer->next("9");
        }
    }
}

void part2()
{
    using namespace publishers;
    using namespace literals;

    auto console = ConsoleSubscriber();
    {
        std::cout << "Simple Return operation:" << std::endl;
        auto jup = Return<std::string>("Jup");
        NOTF_GUARD(jup | console);
        NOTF_GUARD(Return<std::string>("Derbe") | console);
    }
    std::cout << "---" << std::endl;
    {
        std::cout << "Infinite counter with string adapter:" << std::endl;
        auto adapter = Adapter<int, std::string>();
        NOTF_GUARD(Counter<int>(0, 400ms, 100) | adapter);
        NOTF_GUARD(adapter | console);
        getchar();
    }
    std::cout << "---" << std::endl;
    {
        std::cout << "Exhausting generator i => i*2 with i=1, i < 1000:" << std::endl;
        auto generator = Generator<int, std::string>(              //
            1,                                                     // initial state
            [](int& value) { value *= 2; },                        // iterate
            [](const int& value) { return value < 1000; },         // condition
            [](const int& value) { return fmt::to_string(value); } // refine
        );
        NOTF_GUARD(generator | console);
        while (!generator->is_completed()) {
            generator->on_next();
        }
    }
    std::cout << "---" << std::endl;
    {
        std::cout << "Unfolding i => i+2 with i=1, 5 times:" << std::endl;
        auto generator = Generator<int>(    //
            1,                              // initial state
            [](int& value) { value += 2; }, // iterate
            [](const int&) { return true; } // condition
        );
        auto adapter = Adapter<int, std::string>();
        NOTF_GUARD(generator | adapter);
        NOTF_GUARD(adapter | console);
        for (int i = 0; i < 5; ++i) {
            generator->on_next();
        }
    }
    std::cout << "---" << std::endl;
    {
        std::cout << "Using a timer to drive a generator:" << std::endl;
        auto generator_pipeline = Counter<int>(0, 200ms)                                        //
                                  | Adapter<int, NoData>()                                      //
                                  | Generator<double>(1., [](double& value) { value *= 1.01; }) //
                                  | Adapter<double, std::string>()                              //
                                  | console;
        getchar();
    }
}

int main()
{
    part1();
    std::cout << "---------------------" << std::endl;
    part2();
    std::cout << "---------------------" << std::endl;
    std::cout << "Finished" << std::endl;

    return 0;
}
