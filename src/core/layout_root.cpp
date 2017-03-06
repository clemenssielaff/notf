#include "core/layout_root.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/controller.hpp"
#include "core/item.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

const Item* LayoutRootIterator::next()
{
    if (m_layout) {
        const Item* result = m_layout->_get_item();
        m_layout = nullptr;
        return result;
    }
    return nullptr;
}

std::shared_ptr<Window> LayoutRoot::get_window() const
{
    return m_window->shared_from_this();
}

void LayoutRoot::set_item(std::shared_ptr<AbstractController> item)
{
    // remove the existing item first
    if (!is_empty()) {
        const auto& children = _get_children();
        assert(children.size() == 1);
        _remove_child(children.begin()->get());
    }
    _add_child(std::move(item));
    _relayout();

    bool is_redrawn = _redraw();
    assert(is_redrawn);
}

bool LayoutRoot::get_widgets_at(const Vector2f local_pos, std::vector<Widget*>& result)
{
    if (is_empty()) {
        return false;
    }
    return _get_item()->get_layout_item()->get_widgets_at(std::move(local_pos), result);
}

void LayoutRoot::set_render_layer(std::shared_ptr<RenderLayer>)
{
    log_critical << "Cannot change the RenderLayer of the LayoutRoot.";
}

std::unique_ptr<LayoutIterator> LayoutRoot::iter_items() const
{
    return std::make_unique<MakeSmartEnabler<LayoutRootIterator>>(this);
}

void LayoutRoot::_relayout()
{
    if (!is_empty()) {
        _set_item_size(_get_item()->get_layout_item(), get_size());
    }
}

LayoutRoot::LayoutRoot(const std::shared_ptr<Window>& window)
    : Layout()
    , m_window(window.get())
{
    // the layout_root is always in the default render layer
    Layout::set_render_layer(window->get_render_manager().get_default_layer());
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

} // namespace notf
