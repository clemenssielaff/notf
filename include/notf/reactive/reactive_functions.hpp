#pragma once

#include "notf/meta/function.hpp"
#include "notf/reactive/reactive_operator.hpp"

NOTF_OPEN_NAMESPACE

// generator ======================================================================================================== //

/// Generator factory function.
/// Generalizes the workings of a generator into 4 user-defined lambdas.
template<class Policy = detail::DefaultPublisherPolicy, class Data = void, class Iterate = void, class Predicate = void,
         class Refine = void>
auto make_generator(Data&& initial, Iterate&& iterate, Predicate&& predicate, Refine&& refine)
{

    struct Generator : Operator<None, Data, Policy> {
        Generator(Data&& t, Iterate&& it, Predicate&& pre, Refine&& re)
            : m_state(std::forward<Data>(t))
            , m_iterate(std::forward<Iterate>(it))
            , m_predicate(std::forward<Predicate>(pre))
            , m_refine(std::forward<Refine>(re))
        {}

        void on_next(const AnyPublisher*) override
        {
            this->publish(m_refine(m_state));
            if (m_predicate(m_state)) { m_iterate(m_state); }
        }

    private:
        Data m_state;
        Iterate m_iterate;
        Predicate m_predicate;
        Refine m_refine;
    };
    return std::make_shared<Generator>(std::forward<Data>(initial), std::forward<Iterate>(iterate),
                                       std::forward<Predicate>(predicate), std::forward<Refine>(refine));
}

namespace detail {

template<class Policy, class Data, class Iterate>
auto make_generator_iterator_only(Data&& data, Iterate&& iterate)
{
    return make_generator(std::forward<Data>(data),         //
                          std::forward<Iterate>(iterate),   //
                          [](const Data&) { return true; }, // predicate
                          [](const Data& v) { return v; }   // refine
    );
}

template<class Policy, class Data, class Predicate>
auto make_generator_predicate_only(Data&& data, Predicate&& predicate)
{
    return make_generator(std::forward<Data>(data),           //
                          [](Data& v) { ++v; },               // iterate
                          std::forward<Predicate>(predicate), //
                          [](const Data& v) { return v; }     // refine
    );
}

} // namespace detail

template<class Policy = detail::DefaultPublisherPolicy, class Data = void, class Iterate = void>
auto make_generator(Data&& data, Iterate&& iterate)
    -> decltype(function_traits<Iterate>::template is_same<void (&)(Data&)>(),
                detail::make_generator_iterator_only<Policy>(std::forward<Data>(data), std::forward<Iterate>(iterate)))
{
    return detail::make_generator_iterator_only<Policy>(std::forward<Data>(data), std::forward<Iterate>(iterate));
}

template<class Policy = detail::DefaultPublisherPolicy, class Data = void, class Predicate = void>
auto make_generator(Data&& data, Predicate&& predicate)
    -> decltype(function_traits<Predicate>::template is_same<bool (&)(const Data&)>(),
                detail::make_generator_predicate_only<Policy>(std::forward<Data>(data),
                                                              std::forward<Predicate>(predicate)))
{
    return detail::make_generator_predicate_only<Policy>(std::forward<Data>(data), std::forward<Predicate>(predicate));
}

NOTF_CLOSE_NAMESPACE
