#pragma once

#include <memory>

namespace notf {

class Controller;
using ControllerPtr      = std::shared_ptr<Controller>;
using ConstControllerPtr = std::shared_ptr<const Controller>;

class Item;
using ItemPtr      = std::shared_ptr<Item>;
using ConstItemPtr = std::shared_ptr<const Item>;

class Layout;
using LayoutPtr      = std::shared_ptr<Layout>;
using ConstLayoutPtr = std::shared_ptr<const Layout>;

class ScreenItem;
using ScreenItemPtr      = std::shared_ptr<ScreenItem>;
using ConstScreenItemPtr = std::shared_ptr<const ScreenItem>;

class Widget;
using WidgetPtr      = std::shared_ptr<Widget>;
using ConstWidgetPtr = std::shared_ptr<const Widget>;

class Window;

} // namespace notf
