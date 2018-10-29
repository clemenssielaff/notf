#pragma once

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// widget definition ================================================================================================ //

namespace detail {

struct TestWidgetPropertyPolicy {
    using value_t = int;
    static constexpr StringConst name = "test";
    static constexpr value_t default_value = 42;
    static constexpr bool is_visible = true;
};

using WidgetProperties = std::tuple<TestWidgetPropertyPolicy>;

} // namespace detail

// any widget ======================================================================================================= //

class AnyWidget : public CompileTimeNode<detail::WidgetProperties> {

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
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    AnyWidget(valid_ptr<Node*> parent) : CompileTimeNode<detail::WidgetProperties>(parent) {}
};

NOTF_CLOSE_NAMESPACE
