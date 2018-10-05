#pragma once

#include "../meta/pointer.hpp"
#include "./relay.hpp"
#include "./subscriber.hpp"

NOTF_OPEN_NAMESPACE

// pipeline identifier ============================================================================================== //

template<class T>
struct is_pipeline : std::false_type {};
template<class First, class Last>
struct is_pipeline<Pipeline<First, Last>> : std::true_type {};

/// constexpr boolean that is true only if T is a Pipeline
template<class T>
constexpr bool is_pipeline_v = is_pipeline<T>::value;

// pipeline compatibility check ===================================================================================== //

namespace detail {

/// Metafunction that is only defined if P is a Publisher/Pipeline type that can be connected to Subscriber type S.
template<class P, class S, class = void>
struct is_reactive_compatible : std::false_type {};

template<class P, class S>
struct is_reactive_compatible<P, S, std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>)>>
    : std::is_same<typename P::element_type::output_t, typename S::element_type::input_t> {};

template<class P, class S>
struct is_reactive_compatible<P, S, std::enable_if_t<all(is_pipeline_v<P>, is_subscriber_v<S>)>>
    : std::is_same<typename P::last_t::element_type::output_t, typename S::element_type::input_t> {};

/// consexpr boolean that is true iff P is a Publisher type that can be connected to Subscriber type S.
template<class P, class S, class = void>
constexpr bool is_reactive_compatible_v = false;
template<class P, class S>
constexpr bool is_reactive_compatible_v<P, S, decltype(is_reactive_compatible<P, S>(), void())> = true;

} // namespace detail

// pipeline toggle operator ========================================================================================= //

namespace detail {

/// Mixin class for the Toggle Pipeline Operator exposing only enabling / disabling.
class PipelineToggle {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Virtual Destructor.
    virtual ~PipelineToggle() = default;

    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) { m_is_enabled = is_enabled; }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Is the Pipeline enabled or not? (is a size_t in order to avoid padding warnings).
    size_t m_is_enabled = true;
};

/// Operator at the front of every Pipeline.
/// Is used to enable or disable the complete Pipeline and is always at the front, so no Operators within the Pipeline
/// are exucuted, when it is disabled.
template<class T, class Policy = SinglePublisherPolicy>
struct TogglePipelineOperator : public Relay<T, T, Policy>, PipelineToggle {

    /// Propagate the next value if the Pipeline is enabled.
    void on_next(const UntypedPublisher*, const T& value) final
    {
        if (m_is_enabled) {
            this->publish(value);
        }
    }
};

} // namespace detail

// pipeline ========================================================================================================= //

template<class FirstOperator, class LastOperator>
class Pipeline {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of the first Operator in the Pipeline.
    using first_t = FirstOperator;

    /// Type of the last Operator in the Pipeline.
    using last_t = LastOperator;

private:
    /// Special operator used to en/disable the Pipeline.
    using Toggle = std::shared_ptr<detail::PipelineToggle>;

    /// All Operators in the Pipeline (except the last), as untyped reactive operators.
    using Operators = std::vector<std::shared_ptr<UntypedPublisher>>;

    /// This Pipeline type.
    using this_t = Pipeline<FirstOperator, LastOperator>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param first        First Operator in the Pipeline.
    /// @param last         Last Operator in the Pipeline, determines what types can be connected downstream.
    /// @param toggle       Special operator used to en/disable the Pipeline.
    /// @param operators    All Operators in the Pipeline in between the first and last.
    Pipeline(FirstOperator first, LastOperator last, Toggle&& toggle, Operators&& operators = {})
        : m_first(std::forward<FirstOperator>(first))
        , m_last(std::forward<LastOperator>(last))
        , m_toggle(std::move(toggle))
        , m_operators(std::move(operators))
    {}

    /// Access to the first Operator in the Pipeline.
    /// Useful if the first Operator was an r-value that is now owned by the Pipeline.
    FirstOperator& get_first_operator() { return m_first; }

    /// Access to the last Operator in the Pipeline.
    /// Useful if the last Operator was an r-value that is now owned by the Pipeline.
    LastOperator& get_last_operator() { return m_last; }

    size_t get_operator_count()
    {
        size_t result = m_operators.size() + 1;
        if constexpr (std::is_convertible_v<FirstOperator, Toggle>) {
            if (m_first != m_toggle) {
                ++result;
            }
        }
        else {
            ++result;
        }
        if constexpr (std::is_convertible_v<LastOperator, Toggle>) {
            if (m_last != m_toggle) {
                ++result;
            }
        }
        else {
            ++result;
        }
        return result;
    }

    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) { m_toggle->setEnabled(is_enabled); }

    /// Enable the Pipeline.
    void enable() { setEnabled(true); }

    /// Disable the Pipeline.
    void disable() { setEnabled(false); }

    /// 1)
    /// Connect a Pipeline to an l-value Subscriber.
    template<class S>
    std::enable_if_t<all(is_subscriber_v<S>, detail::is_reactive_compatible_v<this_t, S>), this_t&>
    operator|(S& subscriber)
    {
        m_last->subscribe(subscriber);
        return *this;
    }

    /// 2)
    /// Connect a Pipeline to an r-value Subscriber.
    template<class S>
    std::enable_if_t<all(is_subscriber_v<S>, detail::is_reactive_compatible_v<this_t, S>), Pipeline<first_t, S>>
    operator|(S&& subscriber)
    {
        m_last->subscribe(subscriber);
        m_operators.emplace_back(std::move(m_last));
        return Pipeline<first_t, S>(std::move(m_first), std::forward<S>(subscriber), std::move(m_toggle),
                                    std::move(m_operators));
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// First Operator in the Pipeline.
    FirstOperator m_first;

    /// Last Operator in the Pipeline, defines the type of the Pipeline for the purposes of further connection.
    LastOperator m_last;

    /// Special operator used to en/disable the Pipeline.
    /// Can also be the first and/or last.
    Toggle m_toggle;

    /// All Operators in the Pipeline between the first and the last, as untyped reactive operators.
    Operators m_operators;
};

// pipeline pipe operator(s) ======================================================================================== //

/// 3)
/// Connect an l-value Publisher to an l-value Subscriber
template<class P, class S,
         class Relay = std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>
std::enable_if_t<all(is_publisher_v<P>, detail::is_reactive_compatible_v<P, S>), Pipeline<Relay, Relay>>
operator|(P& publisher, S& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(relay, relay, std::move(relay));
}

/// 4)
/// Connect an l-value Publisher to an r-value Subscriber
template<class P, class S,
         class Relay = std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>
std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, detail::is_reactive_compatible_v<P, S>), Pipeline<Relay, S>>
operator|(P& publisher, S&& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(relay, std::forward<S>(subscriber), std::move(relay));
}

/// 5)
/// Connect an r-value Publisher to an l-value Subscriber
template<class P, class S,
         class Relay = std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>
std::enable_if_t<all(is_publisher_v<P>, detail::is_reactive_compatible_v<P, S>), Pipeline<P, Relay>>
operator|(P&& publisher, S& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(std::forward<P>(publisher), relay, std::move(relay));
}

/// 6)
/// Connect an r-value Publisher to an r-value Subscriber
template<class P, class S,
         class Relay = std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>
std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, detail::is_reactive_compatible_v<P, S>), Pipeline<P, S>>
operator|(P&& publisher, S&& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(std::forward<P>(publisher), std::forward<S>(subscriber), std::move(relay));
}

NOTF_CLOSE_NAMESPACE
