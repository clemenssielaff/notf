#pragma once

#include "notf/reactive/pipeline.hpp"

#include "notf/app/property.hpp"

NOTF_OPEN_NAMESPACE

// property handle ================================================================================================== //

/// Object wrapping a weak_ptr to a Property. Is returned by Node::connect_property and can safely be stored &
/// copied anywhere.
template<class T>
class PropertyHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    using property_t = PropertyPtr<T>;

    using operator_t = typename Property<T>::operator_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param property Property to Handle.
    PropertyHandle(const property_t& property) : m_property(property) {}

    /// Reactive Pipeline "|" operator
    /// Connects the Property on the left.
    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<operator_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(const PropertyHandle& property, Sub&& subscriber) {
        property_t property_ptr = property.m_property.lock();
        if (!property_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return property_ptr->get_operator() | std::forward<Sub>(subscriber);
    }

    /// Reactive Pipeline "|" operator
    /// Connect the Property on the right.
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, operator_t>, Pipeline<operator_t>>
    operator|(Pub&& publisher, const PropertyHandle& property) {
        property_t property_ptr = property.m_property.lock();
        if (!property_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return std::forward<Pub>(publisher) | property_ptr->get_operator();
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The handled Property.
    PropertyWeakPtr<T> m_property;
};

NOTF_CLOSE_NAMESPACE
