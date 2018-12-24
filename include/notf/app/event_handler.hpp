#pragma once

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/delegate.hpp"
#include "notf/common/fibers.hpp"
#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// any event ======================================================================================================== //

class AnyEvent {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param weight   Weight of this Event.
    AnyEvent(const float weight = 0) : m_weight(weight) {}

    /// Virtual destructor.
    virtual ~AnyEvent() = default;

    /// The "weight" of this Event.
    float get_weight() const { return m_weight; }

    /// Executes the event function.
    virtual void run() = 0;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The "weight" of an Event is a number that is added to a counter in the Event Handler. Whenver that counter
    /// passes 1.0, a new frame is rendered, even if there are more events in the queue at that point.
    /// The default is 0, meaning that this Event does not add to the counter.
    /// Even if all Events have a counter of zero, the Event handler will draw a new frame eventually once the queue
    /// of new Events is empty ... unless of course you manage to flood it (which should be rather unlikely).
    float m_weight;
};

// event ============================================================================================================ //

template<class Func>
class Event : public AnyEvent {
    static_assert(std::is_invocable_v<Func>, "Events are templated on a callable object without arguments");
    static_assert(std::is_same_v<std::invoke_result_t<Func>, void>, "Events callables must not return a value");

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param function Event function to execute on the UI thread.
    /// @param weight   Weight of this Event (see AnyEvent::m_weight for details).
    Event(Func&& function, const float weight = 0) : AnyEvent(weight), m_function(std::forward<Func>(function)) {}

    /// Executes the event function.
    void run() final {
        if (m_function) { std::invoke(m_function); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    Delegate<void()> m_function;
};

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
    ~EventHandler();

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

// the event handler ================================================================================================ //

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
