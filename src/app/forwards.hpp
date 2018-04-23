#pragma once

#include <memory>

#include "graphics/forwards.hpp"

struct GLFWwindow;
struct GLFWmonitor;

NOTF_OPEN_NAMESPACE

class Application;
class MouseEvent;
class KeyEvent;
class CharEvent;
class WindowEvent;
class SceneNode;

template<typename T>
class Property;

template<typename>
struct NodeHandle;

NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);

NOTF_DEFINE_UNIQUE_POINTERS(class, Renderer);
NOTF_DEFINE_UNIQUE_POINTERS(class, ProceduralRenderer);
NOTF_DEFINE_UNIQUE_POINTERS(class, Scene);
NOTF_DEFINE_UNIQUE_POINTERS(class, Window);
NOTF_DEFINE_UNIQUE_POINTERS(class, SceneManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, PropertyGraph);
NOTF_DEFINE_UNIQUE_POINTERS(class, Event);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, TimerManager);

namespace detail {

class PropertyBase;
struct WindowArguments;

} // namespace detail

namespace test {
struct Harness;
} // namespace test

NOTF_CLOSE_NAMESPACE
