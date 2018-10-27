#pragma once

#include "./property.hpp"

NOTF_OPEN_NAMESPACE

// property handle ================================================================================================== //

template<class T>
class PropertyHandle {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(PropertyHandle);

    /// Property value type.
    using value_t = T;

    /// Type of Operator in this Property.
    using operator_t = typename Property<T>::operator_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    PropertyHandle() = default;

    /// Value Constructor.
    /// @param  property    Property to handle.
    PropertyHandle(PropertyPtr<T> property) : m_property(std::move(property)) {}

    /// @{
    /// Checks whether the PropertyHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_property.expired(); }
    explicit operator bool() const { return !is_expired(); }
    /// @}

    /// The Node-unique name of this Property.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::string_view get_name() const { return _get_property()->get_name(); }

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool is_visible() const { return _get_property()->is_visible(); }

    /// The default value of this Property.
    /// @throws HandleExpiredError  If the Handle has expired.
    const T& get_default() const { return _get_property()->get_default(); }

    /// The Property value.
    /// Blocks until the call can acquire the Graph mutex.
    /// @throws HandleExpiredError  If the Handle has expired.
    const T& get() const
    {
        if (is_expired()) {
            NOTF_THROW(HandleExpiredError, "Property Handle of type '{}' has expired", type_name<T>());
        }
        else {
            NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
            return _get_property()->get();
        }
    }

    /// Blocks until the Graph mutex could be acquired (nonblocking if the thread already holds it) and updates the
    /// Property directly.
    /// @param value                New value.
    /// @throws HandleExpiredError  If the Handle has expired.
    void set(const T& value)
    {
        if (is_expired()) {
            NOTF_THROW(HandleExpiredError, "Property Handle of type '{}' has expired", type_name<T>());
        }
        else {
            NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
            return _get_property()->set(value);
        }
    }

    /// Nonblocking direct update if the current thread already holds the Graph mutex; deferred
    /// update via a `PropertyChangeEvent` if not.
    /// @param value                New value.
    /// @throws HandleExpiredError  If the Handle has expired.
    void set_deferred(const T& value)
    {
        if (is_expired()) {
            NOTF_THROW(HandleExpiredError, "Property Handle of type '{}' has expired", type_name<T>());
        }
        else if (auto possible_lock = std::unique_lock(TheGraph::get_graph_mutex(), std::try_to_lock);
                 possible_lock.owns_lock()) {
            // set immediately if you can
            return _get_property()->set(value);
        }
        else {
            // TODO: property change event
            // TODO: property batches
            return _get_property()->set(value); // this is obviously wrong, just for testing
        }
    }

    /// Comparison operator.
    /// @param rhs  Other PropertyHandle to compare against.
    bool operator==(const PropertyHandle& rhs) const noexcept { return weak_ptr_equal(m_property, rhs.m_property); }
    bool operator!=(const PropertyHandle& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator.
    /// @param rhs  Other PropertyHandle to compare against.
    bool operator<(const PropertyHandle& rhs) const noexcept { return m_property.owner_before(rhs.m_property); }

    // reactive ---------------------------------------------------------------

    /// Connect the Property to a Subscriber downstream.
    /// @throws HandleExpiredError  If the Handle has expired.
    template<class Sub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<operator_t, std::decay_t<Sub>>, Pipeline<std::decay_t<Sub>>>
    operator|(const PropertyHandle& property, Sub&& subscriber)
    {
        return property._get_property()->get_operator() | std::forward<Sub>(subscriber);
    }

    /// Connect a Publisher upstream to a Property.
    /// @throws HandleExpiredError  If the Handle has expired.
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, operator_t>, Pipeline<operator_t>>
    operator|(Pub&& publisher, const PropertyHandle& property)
    {
        return std::forward<Pub>(publisher) | property._get_property()->get_operator();
    }

    /// Connect a Property upstream to a Property downstream.
    friend Pipeline<operator_t> operator|(const PropertyHandle& lhs, const PropertyHandle& rhs)
    {
        return lhs._get_property()->get_operator() | rhs._get_property()->get_operator();
    }

private:
    /// Locks and returns an owning pointer to the handled Property.
    /// @throws HandleExpiredError  If the Handle has expired.
    PropertyPtr<T> _get_property() const
    {
        if (auto property = m_property.lock()) { return property; }
        NOTF_THROW(HandleExpiredError, "Property Handle of type '{}' has expired", type_name<T>());
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The handled Property, non owning.
    PropertyWeakPtr<T> m_property;
};

NOTF_CLOSE_NAMESPACE
