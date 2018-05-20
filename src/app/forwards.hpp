#pragma once

#include "graphics/forwards.hpp"

struct GLFWwindow;
struct GLFWmonitor;

NOTF_OPEN_NAMESPACE

class Application;
class RootSceneNode;
class SceneNode;

// events
struct WindowEvent;
struct WindowResizeEvent;
struct PropertyEvent;
struct MouseEvent;
struct KeyEvent;
struct CharEvent;

template<typename>
struct SceneNodeHandle;

template<class>
class SceneProperty;

NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);
NOTF_DEFINE_SHARED_POINTERS(class, PropertyGraph);
NOTF_DEFINE_SHARED_POINTERS(class, Renderer);
NOTF_DEFINE_SHARED_POINTERS(class, ProceduralRenderer);
NOTF_DEFINE_SHARED_POINTERS(class, Scene);
NOTF_DEFINE_SHARED_POINTERS(class, SceneGraph);

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Property);

NOTF_DEFINE_UNIQUE_POINTERS(class, PropertyHead);
NOTF_DEFINE_UNIQUE_POINTERS(class, Window);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, TimerManager);
NOTF_DEFINE_UNIQUE_POINTERS(struct, Event);

namespace detail {

struct WindowArguments;

} // namespace detail

NOTF_CLOSE_NAMESPACE
