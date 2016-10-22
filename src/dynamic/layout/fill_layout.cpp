#include "dynamic/layout/fill_layout.hpp"

#include <assert.h>

#include "core/widget.hpp"

namespace notf {

FillLayoutItem::~FillLayoutItem()
{
}

std::shared_ptr<Widget> FillLayout::get_widget() const
{
    if (!has_widget()) {
        return {};
    }
    const auto& children = get_children();
    assert(children.size() == 1);
    std::shared_ptr<LayoutObject> obj = children.begin()->second->get_object();
    return std::static_pointer_cast<Widget>(obj);
}

std::shared_ptr<Widget> FillLayout::set_widget(std::shared_ptr<Widget> widget)
{
    // remove the existing item first
    std::shared_ptr<Widget> previous;
    if (has_widget()) {
        const auto& children = get_children();
        assert(children.size() == 1);
        auto it = children.begin();
        previous = std::static_pointer_cast<Widget>(it->second->get_object());
        remove_child(it->first);
    }

    add_item(std::make_unique<MakeSmartEnabler<FillLayoutItem>>(std::move(widget)));
    return previous;
}

std::shared_ptr<Widget> FillLayout::get_widget_at(const Vector2& local_pos) const
{
    if (has_widget()) {
        return get_widget()->get_widget_at(local_pos);
    }
    return {};
}

} // namespace notf