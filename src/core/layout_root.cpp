#include "core/layout_root.hpp"

#include <assert.h>

#include "core/layout_item.hpp"
#include "core/window.hpp"

namespace notf {

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos)
{
    if (has_layout()) {
        return get_layout()->get_widget_at(local_pos);
    }
    return {};
}

void LayoutRoot::set_layout(std::shared_ptr<Layout> item)
{
    // remove the existing item first
    if (has_layout()) {
        const auto& children = get_children();
        assert(children.size() == 1);
        remove_child(children.begin()->first);
    }
    add_child(std::move(item));
}

std::shared_ptr<Layout> LayoutRoot::get_layout() const
{
    if (!has_layout()) {
        return {};
    }
    const auto& children = get_children();
    assert(children.size() == 1);
    std::shared_ptr<LayoutItem> obj = children.begin()->second;
    return std::static_pointer_cast<Layout>(obj);
}

} // namespace notf
