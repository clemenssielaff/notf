#include "core/layout_root.hpp"

#include <assert.h>

#include "core/layout_item.hpp"

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
