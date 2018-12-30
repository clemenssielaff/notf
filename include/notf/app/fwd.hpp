#pragma once

#include "notf/meta/exception.hpp"

#include "notf/common/fwd.hpp"
#include "notf/graphic/fwd.hpp"

// forwards ========================================================================================================= //

struct GLFWwindow;
struct GLFWmonitor;

NOTF_OPEN_NAMESPACE

// application.hpp
namespace detail {
class Application;
}
class TheApplication;
namespace this_thread {
bool is_the_ui_thread();
}

// driver.hpp
class Driver;
namespace driver {
namespace detail {
struct AnyInput;
}
} // namespace driver

// event.hpp
NOTF_DECLARE_UNIQUE_POINTERS(class, AnyEvent);

// event_handler.hpp
class TheEventHandler;

// render_manager.hpp
namespace detail {
class RenderManager;
}
class TheRenderManager;

// graph.hpp
namespace detail {
class Graph;
}
class TheGraph;

// node.hpp
NOTF_DECLARE_SHARED_POINTERS(class, AnyNode);

// node_compiletime.hpp
template<class>
class Node;

// node_handle.hpp
template<class>
class NodeHandle;
template<class>
class NodeOwner;
using AnyNodeHandle = NodeHandle<AnyNode>;
using AnyNodeOwner = NodeOwner<AnyNode>;

// graph/property.hpp
NOTF_DECLARE_SHARED_POINTERS(class, AnyProperty);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, TypedProperty);

// graph/property.hpp
template<class>
class Property;

// root_node.hpp
NOTF_DECLARE_SHARED_POINTERS(class, RootNode);
using RootNodeHandle = NodeHandle<RootNode>;

// scene.hpp
NOTF_DECLARE_SHARED_POINTERS(class, Scene);
class SceneHandle;

// signal.hpp
NOTF_DECLARE_SHARED_POINTERS(class, AnySignal);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, TypedSignal);

// slot.hpp
namespace detail {
template<class>
struct SlotPublisher;
template<class>
struct SlotSubscriber;
} // namespace detail
NOTF_DECLARE_SHARED_POINTERS(class, AnySlot);
NOTF_DECLARE_UNIQUE_POINTERS_TEMPLATE1(class, Slot);
template<class>
class SlotHandle;

// timer_pool.hpp
NOTF_DECLARE_SHARED_POINTERS(class, Timer);
class TheTimerPool;

// visualizer
NOTF_DECLARE_UNIQUE_POINTERS(class, Visualizer);

// window.hpp
namespace detail {
void window_deleter(GLFWwindow* glfw_window);
using GlfwWindowPtr = std::unique_ptr<GLFWwindow, decltype(&window_deleter)>;
} // namespace detail
NOTF_DECLARE_SHARED_POINTERS(class, Window);
class WindowHandle;

// widget //

// widget/any_widget.hpp
class AnyWidget;
class WidgetHandle;

// widget/clipping.hpp
class Clipping;

// widget/painter.hpp
class Painter;

// widget/widget_design.hpp.hpp
class WidgetDesign;

// widget/widget.hpp
template<class, class>
class State;

// widget/widget_scene.hpp
class WidgetScene;
class WidgetSceneHandle;


// widget/painterpreter.hpp
NOTF_DECLARE_UNIQUE_POINTERS(class, Painterpreter);

// exceptions ======================================================================================================= //

/// Exception thrown by any userland Handles when you try to access one when it has already expired.
NOTF_EXCEPTION_TYPE(HandleExpiredError);

/// Error thrown when something went wrong with regards to the Graph hierarchy.
NOTF_EXCEPTION_TYPE(GraphError);

NOTF_CLOSE_NAMESPACE
