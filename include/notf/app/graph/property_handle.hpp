#pragma once

#include "notf/app/graph/property.hpp"

NOTF_OPEN_NAMESPACE

// property handle ================================================================================================== //

/// Object wrapping a weak_ptr to a Property. Is returned by Node::connect_property and can safely be stored &
/// copied anywhere.
template<class T>
class PropertyHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Operator type of the handled Property type.
    using operator_t = typename TypedProperty<T>::operator_t;

    /// Weak pointer to the Property's Operator type.
    using weak_operator_t = typename operator_t::weak_type;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param property Property to Handle.
    PropertyHandle(TypedProperty<T>* property) : m_operator(property->get_operator()) {}

    /// Reactive Pipeline "|" operator
    /// Connects the Property on the left.
    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<operator_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(const PropertyHandle& property, Sub&& subscriber) {
        operator_t operator_ptr = property.m_operator.lock();
        if (!operator_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return operator_ptr | std::forward<Sub>(subscriber);
    }

    /// Reactive Pipeline "|" operator
    /// Connect the Property on the right.
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, operator_t>, Pipeline<operator_t>>
    operator|(Pub&& publisher, const PropertyHandle& property) {
        operator_t operator_ptr = property.m_operator.lock();
        if (!operator_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return std::forward<Pub>(publisher) | operator_ptr;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Operator of the handled Property.
    weak_operator_t m_operator;
};

NOTF_CLOSE_NAMESPACE
