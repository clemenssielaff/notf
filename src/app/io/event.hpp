#pragma once

#include <cstddef>
#include <typeinfo>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Virtual baseclass, can be used as catch-all type for all Events.
struct Event {

    /// Virtual destructor.
    virtual ~Event();

    /// Type of this Event.
    virtual size_t type() const = 0;
};

//====================================================================================================================//

namespace detail {

/// Type to derive Event types via the curiously recurring template pattern.
template<typename T>
struct EventBase : public Event {

    /// Static type of this Event subclass.
    static size_t static_type() { return typeid(T).hash_code(); }

    /// Type of this Event.
    virtual size_t type() const override { return static_type(); }
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
