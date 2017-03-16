#include "core/layout_root.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/controller.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

const Item* LayoutRootIterator::next()
{
    if (m_layout) {
        const Item* result = m_layout->_get_item();
        m_layout           = nullptr;
        return result;
    }
    return nullptr;
}

/**********************************************************************************************************************/

LayoutRoot::LayoutRoot(const std::shared_ptr<Window>& window)
    : Layout()
    , m_window(window.get())
    , m_controller()
{
    // the layout_root is always in the default render layer
    m_render_layer = window->get_render_manager().get_default_layer();
}

std::shared_ptr<Window> LayoutRoot::get_window() const
{
    return m_window->shared_from_this();
}

void LayoutRoot::set_controller(std::shared_ptr<Controller> controller)
{
    // remove the existing item first
    if (!is_empty()) {
        const auto& children = _get_children();
        assert(children.size() == 1);
        _remove_child(children.begin()->get());
    }
    m_controller = controller;
    _add_child(std::move(controller));
    _relayout();
    _redraw();
}

std::unique_ptr<LayoutIterator> LayoutRoot::iter_items() const
{
    return std::make_unique<MakeSmartEnabler<LayoutRootIterator>>(this);
}

std::vector<Widget*> LayoutRoot::get_widgets_at(const Vector2f& screen_pos) const
{
    std::vector<Widget*> result;
    _get_widgets_at(screen_pos, result);
    return result;
}

void LayoutRoot::_relayout()
{
    if (!is_empty()) {
        _set_item_size(m_controller->get_root_item().get(), get_size());
    }
}

Item* LayoutRoot::_get_item() const
{
    if (is_empty()) {
        return nullptr;
    }
    const auto& children = _get_children();
    assert(children.size() == 1);
    return children.begin()->get();
}

void LayoutRoot::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (is_empty()) {
        return;
    }
    _get_widgets_at_item_pos(_get_item(), local_pos, result);
}

} // namespace notf
