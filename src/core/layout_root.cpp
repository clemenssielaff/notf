#include "core/layout_root.hpp"

#include <assert.h>

#include "core/layout_object.hpp"
#include "core/window.hpp"

namespace notf {

LayoutRootItem::~LayoutRootItem()
{
}

std::shared_ptr<Widget> LayoutRoot::get_widget_at(const Vector2& local_pos) const
{
    if (has_layout()) {
        return get_layout()->get_widget_at(local_pos);
    }
    return {};
}

void LayoutRoot::set_layout(std::shared_ptr<AbstractLayout> item)
{
    // remove the existing item first
    if (has_layout()) {
        const auto& children = get_children();
        assert(children.size() == 1);
        remove_child(children.begin()->first);
    }
    add_item(std::make_unique<MakeSmartEnabler<LayoutRootItem>>(std::move(item)));
}

std::shared_ptr<AbstractLayout> LayoutRoot::get_layout() const
{
    if (!has_layout()) {
        return {};
    }
    const auto& children = get_children();
    assert(children.size() == 1);
    std::shared_ptr<LayoutObject> obj = children.begin()->second->get_object();
    return std::static_pointer_cast<AbstractLayout>(obj);
}

} // namespace notf
