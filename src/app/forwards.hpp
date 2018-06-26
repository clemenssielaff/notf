#pragma once

#include "graphics/forwards.hpp"

struct GLFWwindow;
struct GLFWmonitor;

NOTF_OPEN_NAMESPACE

class Application;

// events
NOTF_DEFINE_UNIQUE_POINTERS(struct, Event);

struct WindowEvent;
struct WindowResizeEvent;
struct PropertyEvent;
struct MouseEvent;
struct KeyEvent;
struct CharEvent;
struct FocusEvent;

// scene

NOTF_DEFINE_SHARED_POINTERS(class, Scene);
NOTF_DEFINE_SHARED_POINTERS(class, SceneGraph);

NOTF_DEFINE_SHARED_POINTERS(class, Node);
NOTF_DEFINE_SHARED_POINTERS(class, RootNode);

template<typename>
struct NodeHandle;

// properties

class PropertyHead;
class PropertyGraph;
class PropertyBatch;

template<class T>
class PropertyHandle;

NOTF_DEFINE_SHARED_POINTERS(class, PropertyBody);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, TypedPropertyBody);
NOTF_DEFINE_UNIQUE_POINTERS(struct, PropertyUpdate);

NOTF_DEFINE_SHARED_POINTERS(class, NodeProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, TypedNodeProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, GlobalProperty);

// app

NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, OneShotTimer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);
NOTF_DEFINE_SHARED_POINTERS(class, Renderer);
NOTF_DEFINE_SHARED_POINTERS(class, Window);

NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, TimerManager);

// app / widget

class Widget;

NOTF_DEFINE_SHARED_POINTERS(struct, Capability);

// app / render

NOTF_DEFINE_SHARED_POINTERS(class, Plotter);
NOTF_DEFINE_SHARED_POINTERS(class, ProceduralRenderer);

// tests

#ifdef NOTF_TEST

struct TestNode;

#endif

// details

namespace detail {

struct WindowArguments;

} // namespace detail

NOTF_CLOSE_NAMESPACE
