#pragma once

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/fibers.hpp"
#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/event.hpp"

NOTF_OPEN_NAMESPACE

// event handler ==================================================================================================== //

namespace detail {

class EventHandler {

    friend TheEventHandler;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param buffer_size  Number of items in the Event handling buffer before `schedule` blocks.
    /// @throws ValueError  If the buffer size is zero or not a power of two.
    EventHandler(size_t buffer_size);

    /// Destructor.
    ~EventHandler() { m_event_queue.close(); }

    /// @{
    /// Schedules a new event to be handled on the event thread.
    void schedule(AnyEventPtr&& event) { m_event_queue.push(std::move(event)); }
    template<class Func>
    std::enable_if_t<std::is_invocable_v<Func>> schedule(Func&& function) {
        schedule(std::make_unique<Event<Func>>(std::forward<Func>(function)));
    }
    /// @}

private:
    /// Starts the event handling thread.
    /// @param ui_mutex Mutex turning the event handler thread into the Ui-thread.
    void _start(RecursiveMutex& ui_mutex);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// MPMC queue buffering Events for the event handling thread.
    fibers::buffered_channel<AnyEventPtr> m_event_queue;

    /// Event handling thread.
    Thread m_event_handler;
};

} // namespace detail

// the event handler================================================================================================= //

class TheEventHandler : public ScopedSingleton<detail::EventHandler> {

    friend Accessor<TheEventHandler, detail::Application>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheEventHandler);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(TheEventHandler);

    /// Starts the event handling thread.
    /// @param ui_mutex Mutex turning the event handler thread into the Ui-thread.
    void _start(RecursiveMutex& ui_mutex) { _get()._start(ui_mutex); }

public:
    using ScopedSingleton<detail::EventHandler>::ScopedSingleton;
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheEventHandler, detail::Application> {
    friend detail::Application;

    /// Creates the ScopedSingleton holder instance of TheEventHandler.
    template<class... Args>
    static auto create(Args... args) {
        return TheEventHandler::_create_unique(TheEventHandler::Holder{}, std::forward<Args>(args)...);
    }

    /// Starts the event handling thread.
    /// @param ui_mutex Mutex turning the event handler thread into the Ui-thread.
    static void start(RecursiveMutex& ui_mutex) { TheEventHandler()._start(ui_mutex); }
};

NOTF_CLOSE_NAMESPACE
