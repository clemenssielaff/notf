#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>

#include "core/widget.hpp"

namespace notf {

void StackLayout::add_item(std::shared_ptr<LayoutItem> /*widget*/)
{
}

std::shared_ptr<Widget> StackLayout::get_widget_at(const Vector2& /*local_pos*/)
{
    return {};
}

} // namespace notf
