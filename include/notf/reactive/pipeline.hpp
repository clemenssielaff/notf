#pragma once

#include "./relay.hpp"
#include "./subscriber.hpp"

NOTF_OPEN_NAMESPACE

// pipeline identifier ============================================================================================== //

template<class T>
struct is_pipeline : std::false_type {};
template<class T>
struct is_pipeline<Pipeline<T>> : std::true_type {};

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

    /// Enable the Pipeline.
    void enable() { setEnabled(true); }

    /// Disable the Pipeline.
    void disable() { setEnabled(false); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Is the Pipeline enabled or not? (is a size_t in order to avoid padding warnings).
    size_t m_is_enabled = true;
};

/// Operator at the front of every Pipeline.
/// Is used to enable or disable the complete Pipeline and is always at the front, so no Operators within the Pipeline
/// are exucuted, when it is disabled.
template<class T, class Policy = SinglePublisherPolicy>
struct TogglePipelineOperator : public Relay<T, Policy>, PipelineToggle {

    /// Propagate the next value if the Pipeline is enabled.
    void on_next(const PublisherBase*, const T& value) final
    {
        if (m_is_enabled) {
            this->publish(value);
        }
    }
};

// pipeline ========================================================================================================= //

class PipelineBase {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Special operator used to en/disable the Pipeline.
    using Toggle = std::shared_ptr<PipelineToggle>;

    /// All Operators in the Pipeline (except the last), as untyped reactive operators.
    using Operators = std::vector<std::shared_ptr<PublisherBase>>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @param toggle       Special operator used to en/disable the Pipeline.
    /// @param operators    All Operators in the Pipeline (except the last).
    PipelineBase(Toggle&& toggle, Operators&& operators = {})
        : m_toggle(std::move(toggle)), m_operators(std::move(operators))
    {}

public:
    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) { m_toggle->setEnabled(is_enabled); }

    /// Enable the Pipeline.
    void enable() { setEnabled(true); }

    /// Disable the Pipeline.
    void disable() { setEnabled(false); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Special operator used to en/disable the Pipeline.
    Toggle m_toggle;

    /// All Operators in the Pipeline (except the last), as untyped reactive operators.
    Operators m_operators;
};

} // namespace detail

template<class LastOperator>
class Pipeline : public detail::PipelineBase {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    using typename detail::PipelineBase::Operators;
    using typename detail::PipelineBase::Toggle;

    /// This Pipeline type.
    using this_t = Pipeline<LastOperator>;

public:
    /// Type of the last Operator in the Pipeline.
    using last_t = LastOperator;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param toggle       Special operator used to en/disable the Pipeline.
    /// @param last         Last Operator in the Pipeline, determines the type of this Pipeline.
    /// @param operators    All Operators in the Pipeline (except the last).
    Pipeline(Toggle&& toggle, LastOperator&& last = {}, Operators&& operators = {})
        : detail::PipelineBase(std::move(toggle), std::move(operators)), m_last(std::move(last))
    {}

    /// 1)
    /// Connect a Pipeline to an l-value Subscriber
    template<class S>
    std::enable_if_t<all(is_subscriber_v<S>, detail::is_reactive_compatible_v<this_t, S>), Pipeline<LastOperator>&>
    operator|(S& subscriber)
    {
        m_last->subscribe(subscriber);
        return *this;
    }

    /// 2)
    /// Connect a Pipeline to an r-value Subscriber
    template<class S>
    std::enable_if_t<all(is_subscriber_v<S>, detail::is_reactive_compatible_v<this_t, S>), Pipeline<S>>
    operator|(S&& subscriber)
    {
        m_last->subscribe(subscriber);
        m_operators.emplace_back(std::move(m_last));
        return Pipeline(std::move(m_toggle), std::move(subscriber), std::move(m_operators));
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Last Operator in the Pipeline, defines the type of the Pipeline for the purposes of further connection.
    LastOperator m_last;
};

// pipeline pipe operator(s) ======================================================================================== //

/// 3)
/// Connect an l-value Publisher to an l-value Subscriber
template<class P, class S>
std::enable_if_t<all(is_publisher_v<P>, detail::is_reactive_compatible_v<P, S>),
                 Pipeline<std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>>
operator|(P& publisher, S& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(relay, std::move(relay));
}

/// 4)
/// Connect an l-value Publisher to an r-value Subscriber
template<class P, class S>
std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, detail::is_reactive_compatible_v<P, S>), Pipeline<S>>
operator|(P& publisher, S&& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(relay, std::move(subscriber), {std::move(relay)});
}

/// 5)
/// Connect an r-value Publisher to an l-value Subscriber
template<class P, class S>
std::enable_if_t<all(is_publisher_v<P>, detail::is_reactive_compatible_v<P, S>),
                 Pipeline<std::shared_ptr<detail::TogglePipelineOperator<typename P::element_type::output_t>>>>
operator|(P&& publisher, S& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline(relay, std::move(relay), {std::move(publisher)});
}

/// 6)
/// Connect an r-value Publisher to an r-value Subscriber
template<class P, class S>
std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>, detail::is_reactive_compatible_v<P, S>), Pipeline<S>>
operator|(P&& publisher, S&& subscriber)
{
    using input_t = typename P::element_type::output_t;
    auto relay = std::make_shared<detail::TogglePipelineOperator<input_t>>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);
    return Pipeline<S>(relay, std::move(subscriber), {std::move(publisher), std::move(relay)});
}

NOTF_CLOSE_NAMESPACE
