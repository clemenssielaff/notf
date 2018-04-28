#pragma once

#include <cstddef>
#include <typeinfo>

#include "app/forwards.hpp"
#include "common/meta.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Virtual baseclass, can be used as catch-all type for all Events.
struct Event {

    // methods--------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window   Window that the event is meant for.
    Event(valid_ptr<const Window*> window) : m_window(window) {}

    /// Virtual destructor.
    virtual ~Event();

    /// Type of this Event.
    virtual size_t type() const = 0;

    /// Window that the event is meant for.
    valid_ptr<const Window*> window() const { return m_window; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window that the event is meant for.
    const valid_ptr<const Window*> m_window;
};

//====================================================================================================================//

namespace detail {

/// Type to derive Event types via the curiously recurring template pattern.
template<typename T>
struct EventBase : public Event {

    // methods--------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window   Window that the event is meant for.
    EventBase(valid_ptr<const Window*> window) : Event(window) {}

    /// Static type of this Event subclass.
    static size_t static_type() { return typeid(T).hash_code(); }

    /// Type of this Event.
    virtual size_t type() const override { return static_type(); }
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
