#include "core/layout_root.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/layout_item.hpp"
#include "core/window.hpp"

namespace notf {

void LayoutRoot::set_item(std::shared_ptr<LayoutItem> item)
{
    // remove the existing item first
    if (!is_empty()) {
        const auto& children = get_children();
        assert(children.size() == 1);
        remove_child(children.begin()->first);
    }
    add_child(std::move(item));
}

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos)
{
    if (is_empty()) {
        return {};
    }
    return get_item()->get_widget_at(local_pos);
}

void LayoutRoot::relayout(const Size2r /*size*/)
{
    // TODO: I need a way to update the size of the LayoutRoot whenever the Window's size changes... Signals?
    std::shared_ptr<Window> window = m_window.lock();
    if (!window) {
        log_critical << "Failed to relayout LayoutRoot " << get_handle() << " without a Window";
        return;
    }
    Size2i canvas_size = window->get_canvas_size();
    set_size({Real(canvas_size.width), Real(canvas_size.height)});
    if (!is_empty()) {
        set_item_size(get_item(), get_size());
    }
}

std::shared_ptr<LayoutItem> LayoutRoot::get_item() const
{
    if (is_empty()) {
        return {};
    }
    const auto& children = get_children();
    assert(children.size() == 1);
    return children.begin()->second;
}

} // namespace notf
