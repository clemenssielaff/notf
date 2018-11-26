#pragma once

#include "notf/meta/stringtype.hpp"
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

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Subscriber managing the incoming data to the Slot.
    subscriber_t m_subscriber;

    /// Publisher, publishing the incoming data into the Node.
    publisher_t m_publisher;
};

// compile time slot ================================================================================================ //

namespace detail {

struct SlotPolicyFactory {

    /// Factory method.
    template<class Policy>
    static constexpr auto create() {

        // validate the given Policy and show an appropriate error message if something goes wrong
        static_assert(decltype(has_value_t<Policy>(std::declval<Policy>()))::value,
                      "A SlotPolicy must contain the type of Slot as type `value_t`");

        static_assert(decltype(has_name<Policy>(std::declval<Policy>()))::value,
                      "A SlotPolicy must contain the name of the Slot as `static constexpr name`");
        static_assert(std::is_same_v<decltype(Policy::name), const StringConst>,
                      "The name of a SlotPolicy must be of type `StringConst`");

        /// Validated CompileTimeSlot policy.
        struct SlotPolicy {

            /// Mandatory value type of the Slot Policy.
            using value_t = typename Policy::value_t;

            /// Mandatory name of the Slot Policy.
            static constexpr const StringConst& get_name() { return Policy::name; }
        };

        return SlotPolicy();
    }

    /// Checks, whether the given type has a nested type `value_t`.
    template<class T>
    static constexpr auto has_value_t(const T&) -> decltype(std::declval<typename T::value_t>(), std::true_type{});
    template<class>
    static constexpr auto has_value_t(...) -> std::false_type;

    /// Checks, whether the given type has a static field `name`.
    template<class T>
    static constexpr auto has_name(const T&) -> decltype(T::name, std::true_type{});
    template<class>
    static constexpr auto has_name(...) -> std::false_type;
};

} // namespace detail

/// Example Policy:
///
///     struct ShutdownSlot {
///         using value_t = None;
///         static constexpr StringConst name = "to_shutdown";
///     };
///
template<class Policy>
class CompileTimeSlot final : public Slot<typename Policy::value_t> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Slot type.
    using policy_t = decltype(detail::SlotPolicyFactory::create<Policy>());

    /// Slot value type.
    using value_t = typename policy_t::value_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// The compile time constant name of this Slot.
    static constexpr const StringConst& get_const_name() noexcept { return policy_t::get_name(); }
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

// slot handle ====================================================================================================== //

/// Object wrapping a weak_ptr to a Slot Subscriber. Is returned by Node::get_slot and can safely be stored & copied
/// anywhere. Is also used by Events targeting a specific Slot.
template<class T>
class SlotHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    using subscriber_t = typename Slot<T>::subscriber_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param slot     Slot to Handle.
    SlotHandle(const Slot<T>& slot) : m_subscriber(slot.get_subscriber()) {}

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
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, subscriber_t>, Pipeline<subscriber_t>>
    operator|(Pub&& publisher, SlotHandle& slot) {
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
