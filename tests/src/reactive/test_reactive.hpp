#pragma once

#include "notf/reactive/pipeline.hpp"

namespace {
NOTF_USING_NAMESPACE;

template<class T = int>
auto DefaultSubscriber()
{
    struct DefaultSubscriberImpl : Subscriber<T> {
        void on_next(const AnyPublisher*, const T&) final {}
    };
    return std::make_shared<DefaultSubscriberImpl>();
}

template<class T = int>
auto DefaultPublisher()
{
    return std::make_shared<Publisher<T, detail::SinglePublisherPolicy>>();
}

template<class I = int, class O = I>
auto DefaultOperator()
{
    return std::make_shared<Operator<I, O, detail::SinglePublisherPolicy>>();
}

template<class T = int>
auto TestSubscriber()
{
    struct TestSubscriberTImpl : public Subscriber<T> {

        void on_next(const AnyPublisher*, const T& value) final { values.emplace_back(value); }

        void on_error(const AnyPublisher*, const std::exception& error) final
        {
            try {
                throw error;
            }
            catch (...) {
                exception = std::current_exception();
            };
        }

        void on_complete(const AnyPublisher*) final { is_completed = true; }

        std::vector<T> values;
        std::exception_ptr exception;
        bool is_completed = false;
        bool _padding[7];
    };
    return std::make_shared<TestSubscriberTImpl>();
}

template<>
auto TestSubscriber<None>()
{
    struct TestSubscriberNoneImpl : public Subscriber<None> {

        void on_next(const AnyPublisher*) final { ++counter; }

        void on_error(const AnyPublisher*, const std::exception& error) final
        {
            try {
                throw error;
            }
            catch (...) {
                exception = std::current_exception();
            };
        }

        void on_complete(const AnyPublisher*) final { is_completed = true; }

        size_t counter;
        std::exception_ptr exception;
        bool is_completed = false;
        bool _padding[7];
    };
    return std::make_shared<TestSubscriberNoneImpl>();
}

template<class T = int, class Policy = detail::DefaultPublisherPolicy>
auto TestPublisher()
{
    struct TestPublisherImpl : public Publisher<T, Policy> {
        using parent_t = Publisher<T, Policy>;

        void _error(const std::exception& error) final
        {
            try {
                throw error;
            }
            catch (...) {
                exception = std::current_exception();
            };
            parent_t::_error(error);
        }

        void _complete() final { parent_t::_complete(); }

        void _publish(const T& value) final
        {
            published.emplace_back(value);
            parent_t::_publish(value);
        }

        bool _subscribe(SubscriberPtr<T>& subscriber) final
        {
            if (allow_new_subscribers) { return parent_t::_subscribe(subscriber); }
            else {
                return false;
            }
        }

        std::vector<T> published;
        std::exception_ptr exception;
        bool allow_new_subscribers = true;
        bool _padding[7];
    };
    return std::make_shared<TestPublisherImpl>();
}

} // namespace

template<class Last>
struct notf::Accessor<Pipeline<Last>, Tester> {

    Accessor(Pipeline<Last>& pipeline) : m_pipeline(pipeline) {}

    using Elements = typename Pipeline<Last>::Elements;
    const Elements& get_elements() { return m_pipeline.m_elements; }
    AnyPublisherPtr& get_first() { return m_pipeline.m_first; }
    Last& get_last() { return m_pipeline.m_last; }

    Pipeline<Last>& m_pipeline;
};
template<class Pipe, class P = std::decay_t<Pipe>>
auto PipelinePrivate(Pipe&& pipeline)
{
    return notf::Accessor<Pipeline<typename P::last_t>, Tester>{std::forward<Pipe>(pipeline)};
}
