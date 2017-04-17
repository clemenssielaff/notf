#include "core/window_layout.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/controller.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

const Item* WindowLayoutIterator::next()
{
    if (m_layout) {
        const Item* result = m_layout->m_controller.get();
        m_layout           = nullptr;
        return result;
    }
    return nullptr;
}

/**********************************************************************************************************************/

struct WindowLayout::make_shared_enabler : public WindowLayout {
    template <typename... Args>
    make_shared_enabler(Args&&... args)
        : WindowLayout(std::forward<Args>(args)...) {}
    virtual ~make_shared_enabler();
};
WindowLayout::make_shared_enabler::~make_shared_enabler() {}

std::shared_ptr<WindowLayout> WindowLayout::create(const std::shared_ptr<Window>& window)
{
    return std::make_shared<make_shared_enabler>(window);
}

WindowLayout::WindowLayout(const std::shared_ptr<Window>& window)
    : Layout()
    , m_window(window.get())
    , m_controller()
{
    // the window layout is always in the default render layer
    m_render_layer = window->get_render_manager().get_default_layer();
}

WindowLayout::~WindowLayout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    child_removed(m_controller->get_id());
    _set_item_parent(m_controller.get(), {});
}

bool WindowLayout::has_item(const std::shared_ptr<Item>& item) const
{
    return std::static_pointer_cast<Item>(m_controller) == item;
}

void WindowLayout::clear()
{
    if (m_controller) {
        remove_item(m_controller);
    }
}

void WindowLayout::set_controller(std::shared_ptr<Controller> controller)
{
    if (!controller) {
        log_warning << "Cannot add an empty Controller pointer to a Layout";
        return;
    }

    // remove the existing item first
    if (!is_empty()) {
        if (m_controller == controller) {
            return;
        }
        remove_item(m_controller);
    }

    // Controllers are initialized the first time they are parented to a Layout
    controller->initialize();

    // take ownership of the Item
    _set_item_parent(controller.get(), std::static_pointer_cast<Layout>(shared_from_this()));
    const ItemID child_id = controller->get_id();
    m_controller          = std::move(controller);
    child_added(child_id);

    _relayout();
    _redraw();
}

void WindowLayout::remove_item(const std::shared_ptr<Item>& item)
{
    if (item == m_controller) {
        child_removed(m_controller->get_id());
        m_controller.reset();
    }
    else {
        log_warning << "Could not remove Controller from WindowLayout " << get_id() << " because it is already emppty";
    }
}

std::shared_ptr<Window> WindowLayout::get_window() const
{
    return m_window->shared_from_this();
}

std::unique_ptr<LayoutIterator> WindowLayout::iter_items() const
{
    return std::make_unique<WindowLayoutIterator>(this);
}

std::vector<Widget*> WindowLayout::get_widgets_at(const Vector2f& screen_pos) const
{
    std::vector<Widget*> result;
    _get_widgets_at(screen_pos, result);
    return result;
}

void WindowLayout::_relayout()
{
    if (!is_empty()) {
        _set_item_size(m_controller->get_root_item().get(), get_size());
    }
}

void WindowLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (is_empty()) {
        return;
    }
    _get_widgets_at_item_pos(m_controller.get(), local_pos, result);
}

} // namespace notf
