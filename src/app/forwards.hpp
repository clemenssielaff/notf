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

// NOTF_DEFINE_SHARED_POINTERS(struct, Capability);
// NOTF_DEFINE_SHARED_POINTERS(class, Controller);
// NOTF_DEFINE_SHARED_POINTERS(class, Layout);
// NOTF_DEFINE_SHARED_POINTERS(class, Widget);
// NOTF_DEFINE_SHARED_POINTERS(class, RootLayout);
 NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
// NOTF_DEFINE_SHARED_POINTERS(class, Plotter);
// NOTF_DEFINE_SHARED_POINTERS(class, FragmentProducer);

NOTF_DEFINE_UNIQUE_POINTERS(class, Renderer);
NOTF_DEFINE_UNIQUE_POINTERS(class, Layer);
NOTF_DEFINE_UNIQUE_POINTERS(class, Scene);
NOTF_DEFINE_UNIQUE_POINTERS(class, Window);
NOTF_DEFINE_UNIQUE_POINTERS(class, SceneNode);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, PropertyGraph);
NOTF_DEFINE_UNIQUE_POINTERS(class, RenderThread);
NOTF_DEFINE_UNIQUE_POINTERS(class, Event);
NOTF_DEFINE_UNIQUE_POINTERS(class, EventManager);

namespace detail {

class RenderDag;
class PropertyBase;
struct WindowArguments;

} // namespace detail

NOTF_CLOSE_NAMESPACE
