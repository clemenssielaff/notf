#pragma once

#include <memory>

#include "common/meta.hpp"
#include "graphics/forwards.hpp"

struct GLFWwindow;

namespace notf {

class Application;
class MouseEvent;
class KeyEvent;
class CharEvent;
class FocusEvent;

DEFINE_SHARED_POINTERS(struct, Capability);
DEFINE_SHARED_POINTERS(class, Controller);
DEFINE_SHARED_POINTERS(class, Item);
DEFINE_SHARED_POINTERS(class, Layout);
DEFINE_SHARED_POINTERS(class, ScreenItem);
DEFINE_SHARED_POINTERS(class, Widget);
DEFINE_SHARED_POINTERS(class, Window);
DEFINE_SHARED_POINTERS(class, WindowLayout);
DEFINE_SHARED_POINTERS(class, RenderTarget);
DEFINE_SHARED_POINTERS(class, Layer);
DEFINE_SHARED_POINTERS(class, GraphicsProducer);
DEFINE_SHARED_POINTERS(class, Plotter);
DEFINE_SHARED_POINTERS(class, FragmentProducer);

DEFINE_UNIQUE_POINTERS(class, SceneManager);
DEFINE_UNIQUE_POINTERS(class, ResourceManager);

namespace detail {

DEFINE_UNIQUE_POINTERS(struct, ItemContainer);

} // namespace detail

} // namespace notf
