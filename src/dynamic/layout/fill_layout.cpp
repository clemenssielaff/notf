#include "dynamic/layout/fill_layout.hpp"

#include <assert.h>

#include "core/widget.hpp"

namespace signal {

std::shared_ptr<Widget> FillLayout::get_widget() const
{
    return std::dynamic_pointer_cast<Widget>(get_internal_child());
}

std::shared_ptr<Widget> FillLayout::set_widget(std::shared_ptr<Widget> widget)
{
    std::shared_ptr<Widget> previous_widget = get_widget();
    set_internal_child(widget);
    return previous_widget;
}

std::shared_ptr<Widget> FillLayout::get_widget_at(const Vector2& /*local_pos*/) const
{
    if(!get_internal_child()){
        return {};
    }
    std::shared_ptr<Widget> widget = std::dynamic_pointer_cast<Widget>(get_internal_child());
    assert(widget);
    return widget;
}

void FillLayout::redraw()
{
    if(get_internal_child()){
        get_internal_child()->redraw();
    }
}

} // namespace signal
