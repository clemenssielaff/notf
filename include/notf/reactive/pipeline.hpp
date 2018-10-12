#pragma once

#include "./reactive_operator.hpp"
#include "notf/meta/pointer.hpp"

NOTF_OPEN_NAMESPACE

// pipeline identifier ============================================================================================== //

template<class T>
struct is_pipeline : std::false_type {};
template<class Last>
struct is_pipeline<Pipeline<Last>> : is_subscriber<Last> {};

/// constexpr boolean that is true only if T is a Pipeline
template<class T>
static constexpr const bool is_pipeline_v = is_pipeline<T>::value;

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
static constexpr const bool is_reactive_compatible_v = false;
template<class P, class S>
static constexpr const bool is_reactive_compatible_v<P, S, decltype(is_reactive_compatible<P, S>(), void())> = true;

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
struct TogglePipelineOperator : public Operator<T, T, Policy>, PipelineToggle {

    /// Propagate the next value if the Pipeline is enabled.
    void on_next(const AnyPublisher*, const T& value) final
    {
        if (m_is_enabled) { this->publish(value); }
    }
};

} // namespace detail

// pipeline ========================================================================================================= //

/// A Pipeline connects a single upstream operator with a single downstream operator over 1-n relay operators in
/// between. Every Pipeline has a "toggle" operator (as first or second operator) that it uses to enable and disable the
/// entire Pipeline.
template<class LastOperator>
class Pipeline {

    NOTF_GRANT_TEST_ACCESS(Pipeline)

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of the last Operator in the Pipeline.
    using last_t = LastOperator;

private:
    /// Special operator used to en/disable the Pipeline.
    using Toggle = std::shared_ptr<detail::PipelineToggle>;

    /// All Operators in the Pipeline (except the last), as untyped reactive operators.
    using Operators = std::vector<std::shared_ptr<AnyPublisher>>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param toggle       Special operator used to en/disable the Pipeline.
    /// @param last         Last Operator in the Pipeline, determines what types can be connected downstream.
    /// @param operators    All Operators in the Pipeline in between the first and last.
    Pipeline(Toggle&& toggle, LastOperator last, Operators&& operators = {})
        : m_toggle(std::move(toggle)), m_last(std::forward<LastOperator>(last)), m_operators(std::move(operators))
    {}

    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) { m_toggle->setEnabled(is_enabled); }

    /// Enable the Pipeline.
    void enable() { setEnabled(true); }

    /// Disable the Pipeline.
    void disable() { setEnabled(false); }

    /// Connect a Pipeline to a Subscriber downstream.
    /// @param subscriber   Subscriber downstream to attach to this Pipeline
    /// @returns            New Pipeline object with the new subscriber as "last" operator.
    template<class Sub, class S = std::decay_t<Sub>>
    std::enable_if_t<all(is_subscriber_v<S>, detail::is_reactive_compatible_v<Pipeline<last_t>, S>), Pipeline<S>>
    operator|(Sub&& subscriber)
    {
        m_last->subscribe(subscriber);
        m_operators.emplace_back(std::move(m_last));
        return Pipeline<S>(std::move(m_toggle), std::forward<Sub>(subscriber), std::move(m_operators));
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Special operator used to en/disable the Pipeline.
    /// Can also be the first and/or last.
    Toggle m_toggle;

    /// Last Operator in the Pipeline, defines the type of the Pipeline for the purposes of further connection.
    LastOperator m_last;

    /// All Operators in the Pipeline between the first and the last, as untyped reactive operators.
    Operators m_operators;
};

// pipeline pipe operator(s) ======================================================================================== //

/// Connect a Publisher to a Subscriber
template<class Pub, class Sub, class P = std::decay_t<Pub>, class S = std::decay_t<Sub>>
std::enable_if_t<all(is_publisher_v<P>, detail::is_reactive_compatible_v<P, S>), Pipeline<S>>
operator|(Pub&& publisher, Sub&& subscriber)
{
    auto toggle = std::make_shared<detail::TogglePipelineOperator<typename P::element_type::output_t>>();
    publisher->subscribe(toggle);
    toggle->subscribe(subscriber);
    if constexpr (std::is_lvalue_reference_v<Pub>) {
        return Pipeline<S>(std::move(toggle), std::forward<Sub>(subscriber));
    }
    else {
        return Pipeline<S>(std::move(toggle), std::forward<Sub>(subscriber), {std::forward<Pub>(publisher)});
    }
}

NOTF_CLOSE_NAMESPACE
