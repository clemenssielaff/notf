#include "core/root_layout_item.hpp"

#include "core/layout.hpp"
#include "core/layout_item.hpp"

namespace signal {

std::shared_ptr<Widget> RootLayoutItem::get_widget_at(const Vector2& local_pos) const
{
    if (const std::shared_ptr<LayoutItem>& internal_child = get_internal_child()) {
        return internal_child->get_widget_at(local_pos);
    }
    return {};
}

void RootLayoutItem::set_layout(std::shared_ptr<Layout> item)
{
    set_internal_child(item);
}

void RootLayoutItem::redraw()
{
    if (const std::shared_ptr<LayoutItem>& internal_child = get_internal_child()) {
        internal_child->redraw();
    }
}

} // namespace signal
