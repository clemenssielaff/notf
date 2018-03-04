#pragma once

#include <memory>

#include "common/meta.hpp"
#include "graphics/forwards.hpp"

struct GLFWwindow;

NOTF_OPEN_NAMESPACE

class Application;
class MouseEvent;
class KeyEvent;
class CharEvent;
class FocusEvent;

NOTF_DEFINE_SHARED_POINTERS(struct, Capability);
NOTF_DEFINE_SHARED_POINTERS(class, Controller);
NOTF_DEFINE_SHARED_POINTERS(class, Item);
NOTF_DEFINE_SHARED_POINTERS(class, Layout);
NOTF_DEFINE_SHARED_POINTERS(class, ScreenItem);
NOTF_DEFINE_SHARED_POINTERS(class, Widget);
NOTF_DEFINE_SHARED_POINTERS(class, Window);
NOTF_DEFINE_SHARED_POINTERS(class, WindowLayout);
NOTF_DEFINE_SHARED_POINTERS(class, RenderTarget);
NOTF_DEFINE_SHARED_POINTERS(class, Layer);
NOTF_DEFINE_SHARED_POINTERS(class, GraphicsProducer);
NOTF_DEFINE_SHARED_POINTERS(class, Plotter);
NOTF_DEFINE_SHARED_POINTERS(class, FragmentProducer);
NOTF_DEFINE_SHARED_POINTERS(class, Scene);

NOTF_DEFINE_UNIQUE_POINTERS(class, SceneManager);
NOTF_DEFINE_UNIQUE_POINTERS(class, ResourceManager);

namespace detail {

NOTF_DEFINE_UNIQUE_POINTERS(struct, ItemContainer);

} // namespace detail

NOTF_CLOSE_NAMESPACE
