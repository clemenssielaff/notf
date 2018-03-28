#pragma once

#include <memory>

#include "graphics/forwards.hpp"

struct GLFWwindow;

NOTF_OPEN_NAMESPACE

class Application;

class MouseEvent;
class KeyEvent;
class CharEvent;
class FocusEvent;
class WindowEvent;

template<typename T>
class Property;

NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);

NOTF_DEFINE_UNIQUE_POINTERS(class, Renderer);
NOTF_DEFINE_UNIQUE_POINTERS(class, Scene);
NOTF_DEFINE_UNIQUE_POINTERS(class, Window);
NOTF_DEFINE_UNIQUE_POINTERS(class, SceneManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, SceneNode);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, PropertyGraph);
NOTF_DEFINE_UNIQUE_POINTERS(class, Event);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderManager);

namespace detail {

class PropertyBase;
struct WindowArguments;

} // namespace detail

NOTF_CLOSE_NAMESPACE
