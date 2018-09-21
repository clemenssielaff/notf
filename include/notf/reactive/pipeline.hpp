#pragma once

#include <atomic>
#include <mutex>

#include "../meta/traits.hpp"
#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace detail {

/// Type trait identifying that an end of a Pipeline cannot be connected to anything else.
struct PipelineEndTrait {};

/// Base template of all Pipeline specializations.
template<class T>
struct PipelineBase {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of data flowing through the Pipeline.
    using data_t = T;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(PipelineBase);

    /// Default constructor, produces a disconnected Pipeline.
    PipelineBase() : m_is_connected(false) {}

    /// Value Constructor.
    /// @param subscriber   Subscriber at the "downstream" end of the Pipeline.
    PipelineBase(SubscriberPtr<T> subscriber) : subscriber(std::move(subscriber)), m_is_connected(true) {}

    /// Virtual destructor.
    virtual ~PipelineBase() = default;

    /// Checks if the Pipeline has been disconnected.
    /// Due to uncertainty in a multithreaded environment, it is generally not safe to assume that the Pipeline is
    /// connected if this method returns `false`, as it might have become disconnected in between the check and the
    /// time that the produced boolean is evaluated.
    /// However, if this method returns `true`, you can be certain that this Pipeline has been disconnected.
    bool is_disconnected() const { return !m_is_connected; }

    /// Disconnect the Pipeline.
    virtual void disconnect() = 0;

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// The Subscriber at the "downstream" end of the Pipeline.
    SubscriberPtr<T> subscriber;

protected:
    /// Flag making sure that the Pipeline is disconnected only once.
    std::once_flag m_once_flag;

    /// Flag indicating if the Pipeline has already been disconnected.
    std::atomic_bool m_is_connected;
};

} // namespace detail

// ================================================================================================================== //

template<class T, class I, class O>
struct ExtensionPipeline final : public detail::PipelineBase<T> {

    /// Type flowing through the Pipeline connected to the "upstream" end of this pne.
    using prev_t = I;

    /// Type produced by the Relay at the end of this Pipeline or `PipelineEnd`.
    using next_t = O;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    ExtensionPipeline() = default;

    /// Value Constructor.
    ExtensionPipeline(detail::PipelineBasePtr<I> pipeline, SubscriberPtr<T> subscriber)
        : detail::PipelineBase<T>(std::move(subscriber)), upstream(std::move(pipeline))
    {}

    /// Destructor.
    ~ExtensionPipeline() override { disconnect(); }

    /// Disconnects the Pipeline.
    /// Once disconnected, a Pipeline cannot be reconnected.
    void disconnect() override
    {
        if (this->m_is_connected) {
            std::call_once(this->m_once_flag, [&] {
                this->m_is_connected = false;
                static_cast<Relay<I, T>*>(upstream->subscriber.get())->unsubscribe(this->subscriber);
                this->subscriber.reset();
                upstream.reset();
            });
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// The Pipeline connected to the "upstream" end of this one.
    detail::PipelineBasePtr<I> upstream;
};

template<class T, class I, class O>
struct PublisherPipeline final : public detail::PipelineBase<T> {

    /// Type flowing through the Pipeline connected to the "upstream" end of this pne.
    using prev_t = I;

    /// Type produced by the Relay at the end of this Pipeline or `PipelineEnd`.
    using next_t = O;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    PublisherPipeline() = default;

    /// Value Constructor.
    PublisherPipeline(PublisherPtr<T> publisher, SubscriberPtr<T> subscriber)
        : detail::PipelineBase<T>(std::move(subscriber)), publisher(std::move(publisher))
    {}

    /// Destructor.
    ~PublisherPipeline() override { disconnect(); }

    /// Disconnects the Pipeline.
    /// Once disconnected, a Pipeline cannot be reconnected.
    void disconnect() override
    {
        if (this->m_is_connected) {
            std::call_once(this->m_once_flag, [&] {
                this->m_is_connected = false;
                publisher->unsubscribe(this->subscriber);
                this->subscriber.reset();
                publisher.reset();
            });
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// The Publisher at the "upstream" end of the Pipeline.
    PublisherPtr<T> publisher;
};

// ================================================================================================================== //

namespace detail {

template<typename T, typename = void>
constexpr bool has_input_t = false;
template<typename T>
constexpr bool has_input_t<T, decltype(check_is_type<typename T::input_t>(), void())> = true;

template<typename T, typename = void>
constexpr bool has_output_t = false;
template<typename T>
constexpr bool has_output_t<T, decltype(check_is_type<typename T::output_t>(), void())> = true;

template<typename T, typename = void>
constexpr bool has_prev_t = false;
template<typename T>
constexpr bool has_prev_t<T, decltype(check_is_type<typename T::prev_t>(), void())> = true;

template<typename T, typename = void>
constexpr bool has_next_t = false;
template<typename T>
constexpr bool has_next_t<T, decltype(check_is_type<typename T::next_t>(), void())> = true;

template<class T>
constexpr auto has_just_input_t()
{
    return has_input_t<T> && !has_output_t<T>;
}

template<class T>
constexpr auto has_just_output_t()
{
    return !has_input_t<T> && has_output_t<T>;
}

template<class T>
constexpr auto has_input_and_output_t()
{
    return has_input_t<T> && has_output_t<T>;
}

template<class T>
constexpr auto has_prev_and_next_t()
{
    return has_prev_t<T> && has_next_t<T>;
}

// ------------------------------------

/// Creates a new Pipeline that attaches to a Publisher on the left.
template<class P, class C, class... Args>
auto create_publisher_pipeline(std::shared_ptr<P> publisher, std::shared_ptr<C> subscriber)
{
    using result_t = PublisherPipeline<typename P::output_t, Args...>;
    if (detail::PipelineBasePtr<typename P::output_t> existing_subscription = publisher->get_subscription(subscriber)) {
        return std::static_pointer_cast<result_t>(existing_subscription);
    }
    if (publisher->subscribe(subscriber)) {
        return std::make_shared<result_t>(std::move(publisher), std::move(subscriber));
    }
    return std::make_shared<result_t>();
}

/// Creates a new Pipeline that attaches to another Pipeline on the left.
template<class P, class C, class Next>
auto create_extension_pipeline(std::shared_ptr<P> pipeline, std::shared_ptr<C> subscriber)
{
    using result_t = ExtensionPipeline<typename P::next_t, typename P::data_t, Next>;
    if (static_cast<Relay<typename P::data_t, typename P::next_t>*>(pipeline->subscriber.get())->subscribe(subscriber)) {
        return std::make_shared<result_t>(std::move(pipeline), std::move(subscriber));
    }
    return std::make_shared<result_t>();
}

// ------------------------------------

template<class P, class C>
constexpr bool is_relay_to_relay()
{
    if constexpr (has_input_and_output_t<P>() && has_input_and_output_t<C>()) {
        return all(std::is_base_of_v<Relay<typename P::input_t, typename P::output_t>, P>,
                   std::is_base_of_v<Relay<typename C::input_t, typename C::output_t>, C>);
    }
    return false;
}

template<class P, class C, typename = std::enable_if_t<is_relay_to_relay<P, C>()>>
struct is_relay_to_relay_trait {};

template<class P, class C>
auto relay_to_relay(std::shared_ptr<P> publisher, std::shared_ptr<C> subscriber)
{
    static_assert(std::is_same_v<typename P::output_t, typename C::input_t>,
                  "Cannot create a Pipeline between Relays of different types");
    return create_publisher_pipeline<P, C, typename P::input_t, typename C::output_t>(std::move(publisher),
                                                                                      std::move(subscriber));
}

// ------------------------------------

template<class R, class C>
constexpr bool is_relay_to_subscriber()
{
    if constexpr (has_input_and_output_t<R>() && has_just_input_t<C>()) {
        return all(std::is_base_of_v<Relay<typename R::input_t, typename R::output_t>, R>,
                   std::is_base_of_v<Subscriber<typename C::input_t>, C>);
    }
    return false;
}

template<class R, class C, typename = std::enable_if_t<is_relay_to_subscriber<R, C>()>>
struct is_relay_to_subscriber_trait {};

template<class R, class C>
auto relay_to_subscriber(std::shared_ptr<R> relay, std::shared_ptr<C> subscriber)
{
    static_assert(std::is_same_v<typename R::output_t, typename C::input_t>,
                  "Cannot create a Pipeline between a Relay and Subscriber of different types");
    return create_publisher_pipeline<R, C, typename R::input_t, detail::PipelineEndTrait>(std::move(relay),
                                                                                          std::move(subscriber));
}

// ------------------------------------

template<class P, class R>
constexpr bool is_publisher_to_relay()
{
    if constexpr (has_just_output_t<P>() && has_input_and_output_t<R>()) {
        return all(std::is_base_of_v<Publisher<typename P::output_t>, P>,
                   std::is_base_of_v<Relay<typename R::input_t, typename R::output_t>, R>);
    }
    return false;
}

template<class P, class R, typename = std::enable_if_t<is_publisher_to_relay<P, R>()>>
struct is_publisher_to_relay_trait {};

template<class P, class R>
auto publisher_to_relay(std::shared_ptr<P> publisher, std::shared_ptr<R> relay)
{
    static_assert(std::is_same_v<typename P::output_t, typename R::input_t>,
                  "Cannot create a Pipeline between a Publisher and Relay of different types");
    return create_publisher_pipeline<P, R, detail::PipelineEndTrait, typename R::output_t>(std::move(publisher),
                                                                                           std::move(relay));
}

// ------------------------------------

template<class P, class C>
constexpr bool is_publisher_to_subscriber()
{
    if constexpr (has_just_output_t<P>() && has_just_input_t<C>()) {
        return all(std::is_base_of_v<Publisher<typename P::output_t>, P>,
                   std::is_base_of_v<Subscriber<typename C::input_t>, C>);
    }
    return false;
}

template<class P, class C, typename = std::enable_if_t<is_publisher_to_subscriber<P, C>()>>
struct is_publisher_to_subscriber_trait {};

template<class P, class C>
auto publisher_to_subscriber(std::shared_ptr<P> publisher, std::shared_ptr<C> subscriber)
{
    static_assert(std::is_same_v<typename P::output_t, typename C::input_t>,
                  "Cannot create a Pipeline between a Publisher and Subscriber of different types");
    return create_publisher_pipeline<P, C, detail::PipelineEndTrait, detail::PipelineEndTrait>(std::move(publisher),
                                                                                               std::move(subscriber));
}

// ------------------------------------

template<class P, class R>
constexpr bool is_pipeline_to_relay()
{
    if constexpr (has_prev_and_next_t<P>() && has_input_and_output_t<R>()) {
        return all(std::is_base_of_v<detail::PipelineBase<typename P::data_t>, P>,
                   std::is_base_of_v<Relay<typename R::input_t, typename R::output_t>, R>);
    }
    return false;
}

template<class P, class R, typename = std::enable_if_t<is_pipeline_to_relay<P, R>()>>
struct is_pipeline_to_relay_trait {};

template<class P, class R>
auto pipeline_to_relay(std::shared_ptr<P> pipeline, std::shared_ptr<R> relay)
{
    static_assert(std::is_same_v<typename P::next_t, typename R::input_t>,
                  "Cannot create a Pipeline between a Pipeline and Relay of different types");
    return create_extension_pipeline<P, R, typename R::output_t>(std::move(pipeline), std::move(relay));
}

// ------------------------------------

template<class P, class C>
constexpr bool is_pipeline_to_subscriber()
{
    if constexpr (has_prev_and_next_t<P>() && has_just_input_t<C>()) {
        return all(std::is_base_of_v<detail::PipelineBase<typename P::data_t>, P>,
                   std::is_base_of_v<Subscriber<typename C::input_t>, C>);
    }
    return false;
}

template<class P, class C, typename = std::enable_if_t<is_pipeline_to_subscriber<P, C>()>>
struct is_pipeline_to_subscriber_trait {};

template<class P, class C>
auto pipeline_to_subscriber(std::shared_ptr<P> pipeline, std::shared_ptr<C> subscriber)
{
    static_assert(std::is_same_v<typename P::next_t, typename C::input_t>,
                  "Cannot create a Pipeline between a Pipeline and Subscriber of different types");
    return create_extension_pipeline<P, C, detail::PipelineEndTrait>(std::move(pipeline), std::move(subscriber));
}

} // namespace detail

// ------------------------------------

/// Connect two Relays
template<class P, class C>
auto operator|(std::shared_ptr<P> publisher, std::shared_ptr<C> subscriber)
    -> decltype(detail::is_relay_to_relay_trait<P, C>(), detail::relay_to_relay(publisher, subscriber))
{
    return detail::relay_to_relay(std::move(publisher), std::move(subscriber));
}

/// Connect a Relay to a Subscriber
template<class R, class C>
auto operator|(std::shared_ptr<R> relay, std::shared_ptr<C> subscriber)
    -> decltype(detail::is_relay_to_subscriber_trait<R, C>(), detail::relay_to_subscriber(relay, subscriber))
{
    return detail::relay_to_subscriber(std::move(relay), std::move(subscriber));
}

/// Connect a Publisher to a Relay
template<class P, class R>
auto operator|(std::shared_ptr<P> publisher, std::shared_ptr<R> relay)
    -> decltype(detail::is_publisher_to_relay_trait<P, R>(), detail::publisher_to_relay(publisher, relay))
{
    return detail::publisher_to_relay(std::move(publisher), std::move(relay));
}

/// Connect a Publisher to a Subscriber
template<class P, class C>
auto operator|(std::shared_ptr<P> publisher, std::shared_ptr<C> subscriber)
    -> decltype(detail::is_publisher_to_subscriber_trait<P, C>(),
                detail::publisher_to_subscriber(publisher, subscriber))
{
    return detail::publisher_to_subscriber(std::move(publisher), std::move(subscriber));
}

/// Connect a Pipeline to a Relay
template<class P, class R>
auto operator|(std::shared_ptr<P> pipeline, std::shared_ptr<R> relay)
    -> decltype(detail::is_pipeline_to_relay_trait<P, R>(), detail::pipeline_to_relay(pipeline, relay))
{
    auto result = detail::pipeline_to_relay(pipeline, std::move(relay));
    result->upstream = std::move(pipeline);
    return result;
}

/// Connect a Pipeline to a Subscriber
template<class P, class C>
auto operator|(std::shared_ptr<P> pipeline, std::shared_ptr<C> subscriber)
    -> decltype(detail::is_pipeline_to_subscriber_trait<P, C>(), detail::pipeline_to_subscriber(pipeline, subscriber))
{
    auto result = detail::pipeline_to_subscriber(pipeline, std::move(subscriber));
    result->upstream = std::move(pipeline);
    return result;
}

NOTF_CLOSE_NAMESPACE
