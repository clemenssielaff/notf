#pragma once

#include "notf/meta/pointer.hpp"

#include "notf/app/graph/node_handle.hpp"

NOTF_OPEN_NAMESPACE

// any widget ======================================================================================================= //

class AnyWidget {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Spaces that the transformation of a Widget passes through.
    enum class Space : uchar {
        LOCAL,  // no transformation
        OFFSET, // offset transformation only
        LAYOUT, // layout transformation only
        PARENT, // offset then layout transformation
        WINDOW, // transformation relative to the RootLayout
    };

    /// Error thrown when a requested State transition is impossible.
    NOTF_EXCEPTION_TYPE(BadTransitionError);

    // methods --------------------------------------------------------------------------------- //
public:
    /// Virtual destructor.
    virtual ~AnyWidget() = default;

    /// The name of the current State.
    virtual std::string_view get_state_name() const noexcept = 0;

    /// Checks if a transition from one to the other State is possible.
    virtual bool is_valid_transition(const std::string& from, std::string& to) const = 0;

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    virtual void transition_into(const std::string& state) = 0;
};

// widget =========================================================================================================== //

template<class Base>
class Widget : public Base { // TODO: is `Widget<Base>` a good idea? Right now, we don't need it, but Widget is not
                             // complete yet

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Widget(valid_ptr<AnyNode*> parent) : Base(parent) {}
};

// widget handle ==================================================================================================== //

namespace detail {

template<class Base>
struct NodeHandleInterface<Widget<Base>> : public NodeHandleBaseInterface<Widget<Base>> {};

} // namespace detail

template<class Base>
class WidgetHandle : public NodeHandle<Widget<Base>> {

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using NodeHandle<Widget<Base>>::NodeHandle;
};

NOTF_CLOSE_NAMESPACE
