#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>

#include "core/widget.hpp"

namespace notf {

StackLayoutItem::~StackLayoutItem()
{
}

//std::shared_ptr<Widget> StackLayout::set_widget(std::shared_ptr<Widget> widget)
//{
//    // remove the existing item first
//    std::shared_ptr<Widget> previous;
//    if (has_widget()) {
//        const auto& children = get_children();
//        assert(children.size() == 1);
//        auto it = children.begin();
//        previous = std::static_pointer_cast<Widget>(it->second->get_object());
//        remove_child(it->first);
//    }

//    add_item(std::make_unique<MakeSmartEnabler<FillLayoutItem>>(std::move(widget)));
//    return previous;
//}

std::shared_ptr<Widget> StackLayout::get_widget_at(const Vector2& local_pos) const
{
//    if (has_widget()) {
//        return get_widget()->get_widget_at(local_pos);
//    }
    return {};
}

} // namespace notf
