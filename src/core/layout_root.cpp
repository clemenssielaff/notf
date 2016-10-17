#include "core/layout_root.hpp"

#include "core/layout.hpp"
#include "core/layout_object.hpp"
#include "core/window.hpp"

namespace signal {

Size2r LayoutRoot::get_size() const
{
    Size2i integer_size = get_window()->get_canvas_size();
    return {static_cast<Real>(integer_size.width), static_cast<Real>(integer_size.height)};
}

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos) const
{
    if (const std::shared_ptr<LayoutObject>& internal_child = get_internal_child()) {
        return internal_child->get_widget_at(local_pos);
    }
    return {};
}

void LayoutRoot::set_layout(std::shared_ptr<Layout> item)
{
    set_internal_child(item);
}

void LayoutRoot::redraw()
{
    if (const std::shared_ptr<LayoutObject>& internal_child = get_internal_child()) {
        internal_child->redraw();
    }
}

} // namespace signal
