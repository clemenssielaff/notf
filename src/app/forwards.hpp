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

// scene

NOTF_DEFINE_SHARED_POINTERS(class, Scene);
NOTF_DEFINE_SHARED_POINTERS(class, SceneGraph);

class RootSceneNode;
class SceneNode;

template<typename>
struct SceneNodeHandle;

// properties

template<class>
class PropertyHead;
class PropertyHeadBase;
class PropertyGraph;
class PropertyBatch;
NOTF_DEFINE_SHARED_POINTERS(class, PropertyBodyBase);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, PropertyBody);
NOTF_DEFINE_UNIQUE_POINTERS(struct, PropertyUpdate);
NOTF_DEFINE_UNIQUE_POINTERS_TEMPLATE1(class, SceneProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, GlobalProperty);

//

NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);
NOTF_DEFINE_SHARED_POINTERS(class, Renderer);
NOTF_DEFINE_SHARED_POINTERS(class, ProceduralRenderer);

NOTF_DEFINE_UNIQUE_POINTERS(class, Window);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, TimerManager);

namespace detail {

struct WindowArguments;

} // namespace detail

NOTF_CLOSE_NAMESPACE
