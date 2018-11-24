#pragma once

#include "notf/meta/typename.hpp"

#include "notf/reactive/pipeline.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// any slot ========================================================================================================= //

class AnySlot {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Destructor.
    virtual ~AnySlot() = default;

    /// Name of this Slot type, for runtime reporting.
    virtual std::string_view get_type_name() const = 0;
};

// slot ============================================================================================================= //

/// Is used internally by `Node`.
/// When a specific Slot is requested from the Node, it instead returns the Slot's Subscriber, so the outside world can
/// publish to it, without being able to subscribe.
/// Node subclasses on the other hand, also have access to the Slot's internal Publisher. This way, they can use the "|-
/// operator" to subscribe to updates.
template<class T>
class Slot : public AnySlot {

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
    Slot()
        : m_subscriber(std::make_shared<typename subscriber_t::element_type>(*this))
        , m_publisher(std::make_shared<typename publisher_t::element_type>())
    {}

    /// Name of this Slot type, for runtime reporting.
    std::string_view get_type_name() const final
    {
        static const std::string my_type = type_name<T>();
        return my_type;
    }

    /// Subscriber managing the incoming data to the Slot.
    subscriber_t& get_subscriber() { return m_subscriber; }

    /// Publisher, publishing the incoming data into the Node.
    publisher_t& get_publisher() { return m_publisher; }

//    /// Lets the Node owning this Slots, subscribe to its updates.
//    template<class Sub, class DecayedSub = std::decay_t<Sub>>
//    friend std::enable_if_t<detail::is_reactive_compatible_v<publisher_t, DecayedSub>, Pipeline<DecayedSub>>
//    operator|(const std::shared_ptr<Slot>& slot, Sub&& subscriber)
//    {
//        return slot->get_publisher() | std::forward<Sub>(subscriber);
//    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Subscriber managing the incoming data to the Slot.
    subscriber_t m_subscriber;

    /// Publisher, publishing the incoming data into the Node.
    publisher_t m_publisher;
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
    SlotSubscriber(Slot<T>& slot) : m_slot(slot) {}
    void on_next(const AnyPublisher* id, const T& value) final { m_slot.get_publisher()->publish(id, value); }
    Slot<T>& m_slot;
};
template<>
struct SlotSubscriber<None> : Subscriber<None> {
    SlotSubscriber(Slot<None>& slot) : m_slot(slot) {}
    void on_next(const AnyPublisher* id) final { m_slot.get_publisher()->publish(id); }
    Slot<None>& m_slot;
};
/// @}

} // namespace detail
NOTF_CLOSE_NAMESPACE
