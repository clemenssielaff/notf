#pragma once

#include "notf/meta/concept.hpp"
#include "notf/meta/stringtype.hpp"
#include "notf/meta/typename.hpp"

#include "notf/common/mutex.hpp"

#include "notf/reactive/pipeline.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// any slot ========================================================================================================= //

/// Base class of all Slots.
class AnySlot {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Destructor.
    virtual ~AnySlot() = default;

    /// Name of this Slot type, for runtime reporting.
    virtual std::string_view get_type_name() const = 0;
};

// typed slot ======================================================================================================= //

/// Is used internally by `Node`.
/// When a specific Slot is requested from the Node, it instead returns the Slot's Subscriber, so the outside world can
/// publish to it, without being able to subscribe.
/// Node subclasses on the other hand, also have access to the Slot's internal Publisher. This way, they can use the "|-
/// operator" to subscribe to updates.
template<class T>
class TypedSlot : public AnySlot {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Data type forwarded by the Slot.
    using value_t = T;

    /// Subscriber managing the incoming data to the Slot.
    using subscriber_t = std::shared_ptr<detail::SlotSubscriber<value_t>>;

    /// Publisher, publishing the incoming data into the Node.
    using publisher_t = std::shared_ptr<detail::SlotPublisher<value_t>>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    TypedSlot()
        : m_subscriber(std::make_shared<typename subscriber_t::element_type>(*this))
        , m_publisher(std::make_shared<typename publisher_t::element_type>()) {}

    /// Name of this Slot type, for runtime reporting.
    std::string_view get_type_name() const final {
        static const std::string my_type = type_name<T>();
        return my_type;
    }

    /// @{
    /// Subscriber managing the incoming data to the Slot.
    subscriber_t& get_subscriber() { return m_subscriber; }
    const subscriber_t& get_subscriber() const { return m_subscriber; }
    /// @}

    /// @{
    /// Publisher, publishing the incoming data into the Node.
    publisher_t& get_publisher() { return m_publisher; }
    const publisher_t& get_publisher() const { return m_publisher; }
    /// @}

    /// @{
    /// Manually call the Slot with a given value (if T is not None).
    /// The Publisher id is set to nullptr.
    template<class X = T>
    std::enable_if_t<std::is_same_v<X, None>> call() {
        m_subscriber->on_next(nullptr);
    }
    template<class X = T>
    std::enable_if_t<!std::is_same_v<X, None>> call(const T& value) {
        m_subscriber->on_next(nullptr, value);
    }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Subscriber managing the incoming data to the Slot.
    subscriber_t m_subscriber;

    /// Publisher, publishing the incoming data into the Node.
    publisher_t m_publisher;
};

// slot ============================================================================================================= //

namespace detail {

/// Validates a Slot policy and completes partial policies.
template<class Policy>
class SlotPolicyFactory {

    NOTF_CREATE_TYPE_DETECTOR(value_t);
    static_assert(_has_value_t_v<Policy>, "A SlotPolicy must contain the type of Slot as type `value_t`");

    NOTF_CREATE_FIELD_DETECTOR(name);
    static_assert(_has_name_v<Policy>, "A SlotPolicy must contain the name of the Slot as `static constexpr name`");
    static_assert(std::is_same_v<decltype(Policy::name), const ConstString>,
                  "The name of a SlotPolicy must be of type `ConstString`");

public:
    /// Validated and completed Slot policy.
    struct SlotPolicy {

        /// Mandatory value type of the Slot Policy.
        using value_t = typename Policy::value_t;

        /// Mandatory name of the Slot Policy.
        static constexpr const ConstString name = Policy::name;
    };
};

} // namespace detail

/// Example Policy:
///
///     struct ShutdownSlot {
///         using value_t = None;
///         static constexpr ConstString name = "to_shutdown";
///     };
///
template<class Policy>
class Slot final : public TypedSlot<typename Policy::value_t> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Slot type.
    using policy_t = typename detail::SlotPolicyFactory<Policy>::SlotPolicy;

    /// Slot value type.
    using value_t = typename policy_t::value_t;

    /// The compile time constant name of this Slot.
    static constexpr const ConstString& name = policy_t::name;
};

// slot internals =================================================================================================== //

namespace detail {

/// @{
/// Internal publisher, forwards the identity of the publisher that pushed the value into the Slot.
template<class T>
struct SlotPublisher : Publisher<T, detail::SinglePublisherPolicy> {
    void publish(const AnyPublisher* id, const T& value) { this->_publish(id, value); }
};
template<>
struct SlotPublisher<None> : Publisher<None, detail::SinglePublisherPolicy> {
    void publish(const AnyPublisher* id) { this->_publish(id); }
};
/// @}

/// @{
/// Internal Subscriber, forwards the received value and publisher id to the internal Publisher.
/// Other than a regular reactive Operator, this class is not also a Publisher as well, so the outside world cannot
/// subscribe to a Slot - only its Node.
template<class T>
struct SlotSubscriber : Subscriber<T> {
    SlotSubscriber(TypedSlot<T>& slot) : m_slot(slot) {}
    void on_next(const AnyPublisher* id, const T& value) final {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        m_slot.get_publisher()->publish(id, value);
    }
    TypedSlot<T>& m_slot;
};
template<>
struct SlotSubscriber<None> : Subscriber<None> {
    SlotSubscriber(TypedSlot<None>& slot) : m_slot(slot) {}
    void on_next(const AnyPublisher* id) final {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        m_slot.get_publisher()->publish(id);
    }
    TypedSlot<None>& m_slot;
};
/// @}

} // namespace detail

// slot handle ====================================================================================================== //

/// Object wrapping a weak_ptr to a Slot Subscriber. Is returned by Node::connect_slot and can safely be stored & copied
/// anywhere. Is also used by Events targeting a specific Slot.
template<class T>
class SlotHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    using subscriber_t = typename TypedSlot<T>::subscriber_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param slot     Slot to Handle.
    SlotHandle(const TypedSlot<T>* slot) : m_subscriber(slot->get_subscriber()) {}

    /// Manually call the Slot with a given value (if applicable).
    template<class X = T>
    std::enable_if_t<std::is_same_v<X, None>> call() {
        subscriber_t subscriber = m_subscriber.lock();
        if (!subscriber) { NOTF_THROW(HandleExpiredError, "SlotHandle is expired"); }
        subscriber->on_next(nullptr);
    }
    template<class X = T>
    std::enable_if_t<!std::is_same_v<X, None>> call(const T& value) {
        subscriber_t subscriber = m_subscriber.lock();
        if (!subscriber) { NOTF_THROW(HandleExpiredError, "SlotHandle is expired"); }
        subscriber->on_next(nullptr, value);
    }

    /// Reactive Pipeline "|" operator
    /// Connect the Slot on the right.
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, subscriber_t>, Pipeline<subscriber_t>>
    operator|(Pub&& publisher, const SlotHandle& slot) {
        subscriber_t subscriber = slot.m_subscriber.lock();
        if (!subscriber) { NOTF_THROW(HandleExpiredError, "SlotHandle is expired"); }
        return std::forward<Pub>(publisher) | subscriber;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The handled Slot.
    std::weak_ptr<detail::SlotSubscriber<T>> m_subscriber;
};

NOTF_CLOSE_NAMESPACE
