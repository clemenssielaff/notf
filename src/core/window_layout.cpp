#include "core/window_layout.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/controller.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

Item* WindowLayoutIterator::next()
{
    if (m_layout) {
        Item* result = m_layout->m_controller.get();
        m_layout     = nullptr;
        return result;
    }
    return nullptr;
}

/**********************************************************************************************************************/

WindowLayoutPtr WindowLayout::create(const std::shared_ptr<Window>& window)
{
    struct make_shared_enabler : public WindowLayout {
        make_shared_enabler(const std::shared_ptr<Window>& window)
            : WindowLayout(window) {}
    };
    return std::make_shared<make_shared_enabler>(window);
}

WindowLayout::WindowLayout(const std::shared_ptr<Window>& window)
    : Layout()
    , m_window(window.get())
    , m_controller()
{
    // the window layout is always in the default render layer
    m_has_own_render_layer = true;
    m_render_layer         = window->get_render_manager().get_default_layer();
}

WindowLayout::~WindowLayout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    on_child_removed(m_controller->get_id());
    _set_parent(m_controller.get(), {});
}

bool WindowLayout::has_item(const ItemPtr& item) const
{
    return std::static_pointer_cast<Item>(m_controller) == item;
}

void WindowLayout::clear()
{
    if (m_controller) {
        remove_item(m_controller);
    }
}

void WindowLayout::set_controller(ControllerPtr controller)
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

    // take ownership of the Item
    _set_parent(controller.get(), make_shared_from(this));
    const ItemID child_id = controller->get_id();
    m_controller          = std::move(controller);
    on_child_added(child_id);

    // Controllers are initialized the first time they are parented to a Layout
    m_controller->initialize();

    _relayout();
    _redraw();
}

void WindowLayout::remove_item(const ItemPtr& item)
{
    if (item == m_controller) {
        on_child_removed(m_controller->get_id());
        m_controller.reset();
    }
    else {
        log_warning << "Could not remove Controller from WindowLayout " << get_id() << " because it is already emppty";
    }
}

Aabrf WindowLayout::get_content_aabr() const
{
    if (m_controller) {
        if (ScreenItem* screen_item = get_screen_item(m_controller.get())) {
            return screen_item->get_aarbr();
        }
    }
    return Aabrf::zero();
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
        _set_size(m_controller->get_root_item().get(), get_size());
    }
    on_layout_changed();
}

void WindowLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (is_empty()) {
        return;
    }
    Item::_get_widgets_at(m_controller.get(), local_pos, result);
}

Claim WindowLayout::_aggregate_claim()
{
    const Size2f window_size = m_window->get_window_size();
    return Claim::fixed(window_size.width, window_size.height);
}

} // namespace notf
