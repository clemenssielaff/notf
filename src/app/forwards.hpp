#pragma once

#include "graphics/forwards.hpp"

struct GLFWwindow;
struct GLFWmonitor;

NOTF_OPEN_NAMESPACE

// app ============================================================================================================== //

class Application;
class PropertyHead;
class PropertyGraph;
class PropertyBatch;

template<class T>
class PropertyHandle;

template<typename>
struct NodeHandle;

NOTF_DEFINE_SHARED_POINTERS(class, Scene);
NOTF_DEFINE_SHARED_POINTERS(class, SceneGraph);
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, OneShotTimer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);
NOTF_DEFINE_SHARED_POINTERS(class, Window);
NOTF_DEFINE_SHARED_POINTERS(class, PropertyBody);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, TypedPropertyBody);
NOTF_DEFINE_UNIQUE_POINTERS(struct, PropertyUpdate);

NOTF_DEFINE_SHARED_POINTERS(class, Node);
NOTF_DEFINE_SHARED_POINTERS(class, RootNode);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, TimerManager);
NOTF_DEFINE_SHARED_POINTERS(class, NodeProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, TypedNodeProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, GlobalProperty);

namespace detail {
struct WindowSettings;
} // namespace detail

// app/io =========================================================================================================== //

struct WindowEvent;
struct WindowResizeEvent;
struct PropertyEvent;
struct MouseEvent;
struct KeyEvent;
struct CharEvent;
struct FocusEvent;

NOTF_DEFINE_UNIQUE_POINTERS(struct, Event);

// app/widget ======================================================================================================= //

class Widget;
class WidgetDesign;
class WidgetVisualizer;
class Painter;

NOTF_DEFINE_UNIQUE_POINTERS(class, Painterpreter);

NOTF_DEFINE_SHARED_POINTERS(struct, Capability);
NOTF_DEFINE_SHARED_POINTERS(class, WidgetScene);

// app/visualizer =================================================================================================== //

NOTF_DEFINE_UNIQUE_POINTERS(class, Visualizer);

NOTF_DEFINE_SHARED_POINTERS(class, Plate);

// tests ============================================================================================================ //

#ifdef NOTF_TEST

struct TestNode;

#endif

NOTF_CLOSE_NAMESPACE
