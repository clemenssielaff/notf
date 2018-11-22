#pragma once

#include <cstddef>
#include <typeinfo>

#include "notf/app/property_handle.hpp"

NOTF_OPEN_NAMESPACE

// any event ======================================================================================================== //

/// Virtual baseclass, can be used as catch-all type for all Events.
class AnyEvent {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Virtual destructor.
    virtual ~AnyEvent() = default;

    /// Type of this Event.
    virtual size_t get_type() const = 0;

    /// Node that the event is meant for.
    virtual NodeHandle get_node() const;
};

// ================================================================================================================== //

namespace detail {

/// Type to derive Event types via the curiously recurring template pattern.
template<class Deriv>
class EventBase : public AnyEvent {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Static type of this Event subclass.
    static size_t get_static_type() { return typeid(Deriv).hash_code(); }

    /// Type of this Event.
    size_t get_type() const override { return get_static_type(); }
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
