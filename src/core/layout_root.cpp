#include "core/layout_root.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/transform2.hpp"
#include "core/layout_item.hpp"
#include "core/window.hpp"

namespace notf {

void LayoutRoot::set_item(std::shared_ptr<LayoutItem> item)
{
    // remove the existing item first
    if (!is_empty()) {
        const auto& children = _get_children();
        assert(children.size() == 1);
        _remove_child(children.begin()->first);
    }
    _add_child(std::move(item));
    _relayout({});
}

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos)
{
    if (is_empty()) {
        return {};
    }
    return _get_item()->get_widget_at(local_pos);
}

void LayoutRoot::_relayout(const Size2r /*size*/)
{
    std::shared_ptr<Window> window = m_window.lock();
    if (!window) {
        log_critical << "Failed to relayout LayoutRoot " << get_handle() << " without a Window";
        return;
    }
    Size2i canvas_size = window->get_buffer_size();
    _set_size({Real(canvas_size.width), Real(canvas_size.height)});
    if (!is_empty()) {
        _update_item(_get_item(), get_size(), Transform2::identity());
    }
}

std::shared_ptr<LayoutItem> LayoutRoot::_get_item() const
{
    if (is_empty()) {
        return {};
    }
    const auto& children = _get_children();
    assert(children.size() == 1);
    return children.begin()->second;
}

} // namespace notf
