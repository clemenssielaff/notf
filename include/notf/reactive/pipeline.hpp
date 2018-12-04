#pragma once

#include "notf/meta/pointer.hpp"
#include "notf/meta/typename.hpp"

#include "notf/reactive/operator.hpp"

NOTF_OPEN_NAMESPACE

// pipeline identifier ============================================================================================== //

/// @{
/// Constexpr boolean that is true only if T is a typed Pipeline.
template<class T>
struct is_typed_pipeline : std::false_type {};
template<class Last>
struct is_typed_pipeline<Pipeline<Last>> : is_publisher<Last> {};
template<class T>
static constexpr bool is_typed_pipeline_v = is_typed_pipeline<T>::value;
/// @}

/// @{
/// Constexpr boolean that is true only if T is an untyped Pipeline.
template<class T>
struct is_untyped_pipeline : std::false_type {};
template<>
struct is_untyped_pipeline<Pipeline<AnyPublisherPtr>> : std::true_type {};
template<>
struct is_untyped_pipeline<Pipeline<AnyOperatorPtr>> : std::true_type {};
template<class T>
static constexpr bool is_untyped_pipeline_v = is_untyped_pipeline<T>::value;
/// @}

/// Constexpr boolean that is true only of T is any (typed or untyped) Pipeline.
template<class T>
static constexpr bool is_pipeline_v = is_typed_pipeline_v<T> || is_untyped_pipeline_v<T>;

// pipeline compatibility check ===================================================================================== //

namespace detail {

/// Metafunction that is only defined if P is a Publisher/Pipeline type that can be connected to Subscriber type S.
template<class P, class S, class = void>
struct is_reactive_compatible : std::false_type {};

// typed publisher -> typed subscriber
template<class P, class S>
struct is_reactive_compatible<P, S, std::enable_if_t<all(is_publisher_v<P>, is_subscriber_v<S>)>>
    : std::disjunction<std::is_same<typename P::element_type::output_t, typename S::element_type::input_t>,
                       std::is_same<All, typename S::element_type::input_t>> {};

// typed pipeline -> typed subscriber
template<class P, class S>
struct is_reactive_compatible<P, S, std::enable_if_t<all(is_typed_pipeline_v<P>, is_subscriber_v<S>)>>
    : std::disjunction<std::is_same<typename P::last_t::element_type::output_t, typename S::element_type::input_t>,
                       std::is_same<All, typename S::element_type::input_t>> {};

// typed publisher/pipeline -> untyped subscriber
template<class P, class S>
struct is_reactive_compatible<
    P, S,
    std::enable_if_t<all(any(is_publisher_v<P>, is_typed_pipeline_v<P>),
                         any(std::is_same_v<S, AnyOperatorPtr>, std::is_same_v<S, AnySubscriberPtr>))>>
    : std::true_type {};

// untyped publisher -> all subscribers
template<class P, class S>
struct is_reactive_compatible<
    P, S,
    std::enable_if_t<all(any(std::is_same_v<P, AnyOperatorPtr>, std::is_same_v<P, AnyPublisherPtr>),
                         any(std::is_same_v<S, AnyOperatorPtr>, std::is_same_v<S, AnySubscriberPtr>,
                             is_subscriber_v<S>))>> : std::false_type {
    static_assert(always_false_v<P>, "The publisher at the top of a pipeline must be statically typed");
};

// untyped pipeline -> all subscribers
template<class P, class S>
struct is_reactive_compatible<
    P, S,
    std::enable_if_t<all(is_untyped_pipeline_v<P>, any(std::is_same_v<S, AnyOperatorPtr>,
                                                       std::is_same_v<S, AnySubscriberPtr>, is_subscriber_v<S>))>>
    : std::true_type {};

/// consexpr boolean that is true iff P is a Publisher type that can be connected to Subscriber type S.
template<class P, class S>
static constexpr bool is_reactive_compatible_v = is_reactive_compatible<P, S>::value;

} // namespace detail

// pipeline toggle operator ========================================================================================= //

namespace detail {

/// Mixin class for the Toggle Pipeline Operator exposing only enabling / disabling.
class PipelineToggle {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Virtual Destructor.
    virtual ~PipelineToggle() = default;

    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) { m_is_enabled = is_enabled; }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Is the Pipeline enabled or not? (is a size_t in order to avoid padding warnings).
    bool m_is_enabled = true;
};

/// Operator near the front of every Pipeline.
/// Is used to enable or disable the complete pipeline and is always at the front, so no other operators within the
/// pipeline are exucuted, when it is disabled.
template<class T, class Policy = SinglePublisherPolicy>
struct TogglePipelineOperator : public Operator<T, T, Policy>, PipelineToggle {

    /// Propagate the next value if the Pipeline is enabled.
    void on_next(const AnyPublisher* publisher, const T& value) final {
        if (m_is_enabled && !this->is_completed()) { this->_publish(publisher, value); }
    }
};
template<class Policy>
struct TogglePipelineOperator<None, Policy> : public Operator<None, None, Policy>, PipelineToggle {

    /// Propagate the next signal if the Pipeline is enabled.
    void on_next(const AnyPublisher* publisher) final {
        if (m_is_enabled && !this->is_completed()) { this->_publish(publisher); }
    }
};

} // namespace detail

// pipeline error =================================================================================================== //

/// Exception thrown when a Pipeline could not be connected.
NOTF_EXCEPTION_TYPE(PipelineError);

// pipeline ========================================================================================================= //

/// Pipeline base class, used to store multiple Pipelines in the same Container.
class AnyPipeline {

    // methods --------------------------------------------------------------------------------- //
public:
    virtual ~AnyPipeline() = default;

    /// Explicitly enable/disable the Pipeline.
    virtual void setEnabled(const bool is_enabled) = 0;

    /// Enable the Pipeline.
    void enable() { setEnabled(true); }

    /// Disable the Pipeline.
    void disable() { setEnabled(false); }
};

/// A Pipeline connects a single upstream publisher with a daisy-chain of downstream subscribers.
/// Every Pipeline has a "toggle" operator (as first or second function in the chain) that it uses to enable and disable
/// the entire Pipeline.
template<class Last>
class Pipeline : public AnyPipeline {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Pipeline);

    /// Type of the last function in the pipeline.
    using last_t = Last;

private:
    /// Special operator used to en/disable the Pipeline.
    using Toggle = std::shared_ptr<detail::PipelineToggle>;

    /// All functions in the pipeline, as untyped reactive subscribers.
    using Functions = std::vector<AnySubscriberPtr>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Pipeline);

    /// Constructor.
    /// @param toggle       Special operator used to en/disable the Pipeline.
    /// @param last         Last subscriber in the Pipeline, determines what types can be connected downstream.
    /// @param first        Publisher at the top of the pipeline, if it is an r-value that would otherwise expire.
    /// @param functions    All functions in the Pipeline in between the first and last (excluding the toggle).
    Pipeline(Toggle&& toggle, Last last, AnyPublisherPtr first = {}, Functions&& functions = {})
        : m_first(std::move(first))
        , m_toggle(std::move(toggle))
        , m_last(std::forward<Last>(last))
        , m_functions(std::move(functions)) {}

    /// Move Constructor.
    Pipeline(Pipeline&& other) = default;

    /// Explicitly enable/disable the Pipeline.
    void setEnabled(const bool is_enabled) final { m_toggle->setEnabled(is_enabled); }

    /// Connect a Pipeline to a Subscriber downstream.
    /// @param subscriber   Subscriber downstream to attach to this Pipeline
    /// @returns            New Pipeline object with the new subscriber as "last" operator.
    template<class Sub, class S = std::decay_t<Sub>,
             class = std::enable_if_t<detail::is_reactive_compatible_v<Pipeline<last_t>, S>>>
    auto operator|(Sub&& rhs) {
        // AnyOperators have to be cast to AnySubscribers before we can make use of them
        auto subscriber = [&rhs]() {
            if constexpr (std::is_same_v<S, AnyOperatorPtr>) {
                return std::dynamic_pointer_cast<AnySubscriber>(rhs);
            } else {
                return rhs; // creates a copy
            }
        }();
        if (!subscriber) { NOTF_THROW(PipelineError, "Cannot connect an empty operator to a pipeline"); }

        // if the pipeline is untyped, we have to first make sure that its last function can act as a publisher
        if constexpr (is_one_of_v<last_t, AnyOperatorPtr, AnySubscriberPtr>) {
            auto publisher = std::dynamic_pointer_cast<AnyPublisher>(m_last);
            NOTF_ASSERT(publisher); // there should be no way this fails, if we did the SFINAE magic correctly
            if (!publisher->subscribe(std::move(subscriber))) {
                NOTF_THROW(PipelineError, "Publisher of type \"AnyPublisher\" rejected subscriber #{} of type \"{}\"",
                           publisher->get_subscriber_count(), type_name<typename S::element_type>());
            }

            // even though every operator should be a subscriber, we have to dynamically cast them
            if constexpr (std::is_same_v<last_t, AnyOperatorPtr>) {
                auto last_as_subscriber = std::dynamic_pointer_cast<AnySubscriber>(m_last);
                NOTF_ASSERT(last_as_subscriber);
                m_functions.emplace_back(std::move(last_as_subscriber));
            } else {
                m_functions.emplace_back(std::move(m_last));
            }
        }

        // if everything is typed, we don't need to cast at all
        else {
            NOTF_ASSERT_ALWAYS(m_last->subscribe(std::move(subscriber))); // shouldn't even compile if it could fail
            m_functions.emplace_back(std::move(m_last));
        }

        return Pipeline<S>(std::move(m_toggle), std::forward<Sub>(rhs), std::move(m_first), std::move(m_functions));
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Publisher at the top of the pipeline, if it is an r-value that would otherwise expire.
    AnyPublisherPtr m_first;

    /// Used to en/disable the pipeline, either the first operator in the pipeline or right after `m_first`..
    Toggle m_toggle;

    /// Last function in the pipeline, defines the type of the pipeline for the purposes of further connection.
    Last m_last;

    /// All functions in the Pipeline in between the first and last (excluding the toggle)
    Functions m_functions;
};

// make pipeline ==================================================================================================== //

/// Stores a Pipeline of arbitrary type into a AnyPipelinePtr (unique_ptr<AnyPipeline>).
/// @param pipeline Pipeline to store.
template<class Pipe, class = std::enable_if_t<std::conjunction_v<std::is_base_of<AnyPipeline, Pipe>>>>
AnyPipelinePtr make_pipeline(Pipe&& pipeline) {
    return std::make_unique<Pipe>(std::forward<Pipe>(pipeline));
}

// pipeline pipe operator(s) ======================================================================================== //

/// Connect a Publisher to a Subscriber
template<class Pub, class Sub, class P = std::decay_t<Pub>, class S = std::decay_t<Sub>>
std::enable_if_t<!is_pipeline_v<P> && detail::is_reactive_compatible_v<P, S>, Pipeline<S>>
operator|(Pub&& publisher, Sub&& subscriber) {
    if (!publisher || !subscriber) { NOTF_THROW(PipelineError, "Cannot connect a nullptr to a pipeline"); }

    // this always succeeds, as we create the toggle specifically for the given publisher
    using pipe_t = typename P::element_type::output_t;
    auto toggle = std::make_shared<detail::TogglePipelineOperator<pipe_t>>();
    if (!publisher->subscribe(toggle)) {
        NOTF_THROW(PipelineError, "Failed to connect Pipeline. Does the Publisher only accept a single Subscriber?");
    }

    // if the subscriber is untyped, we have to dynamically cast it which may fail
    if constexpr (is_one_of_v<S, AnyOperatorPtr, AnySubscriberPtr>) {
        auto typed_subscriber = std::dynamic_pointer_cast<Subscriber<pipe_t>>(subscriber);
        if (!typed_subscriber) {
            NOTF_THROW(PipelineError,
                       "Could not cast object of type \"{}\" to \"{}\" in order to connect it to a pipeline",
                       type_name<typename S::element_type>(), type_name<Subscriber<pipe_t>>());
        }
        NOTF_ASSERT_ALWAYS(toggle->subscribe(std::move(typed_subscriber)));
    } else {
        NOTF_ASSERT_ALWAYS(toggle->subscribe(subscriber));
    }

    // r-value publishers are captured inside the pipeline
    if constexpr (std::is_lvalue_reference_v<Pub>) {
        return Pipeline<S>(std::move(toggle), std::forward<Sub>(subscriber));
    } else {
        return Pipeline<S>(std::move(toggle), std::forward<Sub>(subscriber), std::forward<Pub>(publisher));
    }
}

// TODO: what abut NoDag errors in a reactive system?

NOTF_CLOSE_NAMESPACE
