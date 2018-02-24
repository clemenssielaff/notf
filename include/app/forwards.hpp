#pragma once

#include <memory>

#include "common/meta.hpp"

struct GLFWwindow;

namespace notf {

class Application;
class MouseEvent;
class KeyEvent;
class CharEvent;

DEFINE_SHARED_POINTERS(class, Window);
DEFINE_SHARED_POINTERS(class, WindowLayout);

DEFINE_UNIQUE_POINTERS(class, ResourceManager);

} // namespace notf
