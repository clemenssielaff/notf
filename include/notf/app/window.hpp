#pragma once

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// window =========================================================================================================== //

namespace detail {
using WindowProperties = std::tuple< //
    >;
} // namespace detail

class Window : public CompileTimeNode<detail::WindowProperties> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Compile time Node base type.
    using super_t = CompileTimeNode<detail::WindowProperties>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Window(valid_ptr<Node*> parent) : super_t(parent) {}
};

// window handle ==================================================================================================== //

class WindowHandle : public TypedNodeHandle<Window> {
    // methods --------------------------------------------------------------------------------- //
public:

    void close(){} // TODO: Window
};

NOTF_CLOSE_NAMESPACE
