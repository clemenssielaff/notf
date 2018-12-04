#pragma once

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// any event ======================================================================================================== //

class AnyEvent {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Virtual destructor.
    virtual ~AnyEvent() = default;

    /// Executes the event function.
    virtual void run() = 0;
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
    Event(Func&& function) : m_function(std::forward<Func>(function)) {}

    /// Executes the event function.
    void run() final {
        if (m_function) { std::invoke(m_function); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    std::function<void()> m_function;
};

NOTF_CLOSE_NAMESPACE
