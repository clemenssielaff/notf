#include "dynamic/layout/flexbox_layout.hpp"

#include "core/widget.hpp"

namespace signal {

FlexboxLayoutComponent::FlexboxLayoutComponent(std::shared_ptr<Widget> owner,
                                               DIRECTION direction, DIRECTION wrap_direction)
    : LayoutComponent(owner)
    , m_items()
    , m_direction(direction)
    , m_wrap_direction(wrap_direction == DIRECTION::INVALID ? direction : wrap_direction)
{
}

void FlexboxLayoutComponent::add_widget(std::shared_ptr<Widget> widget)
{
    m_items.emplace_back(std::make_shared<FlexboxLayoutItem>(shared_from_this(), std::move(widget)));
    update();
}

std::shared_ptr<LayoutItem> FlexboxLayoutComponent::item_at(const Vector2& local_pos)
{
    return {};
}

void FlexboxLayoutComponent::update()
{

}

FlexboxLayoutItem::FlexboxLayoutItem(std::shared_ptr<FlexboxLayoutComponent> layout, std::shared_ptr<Widget> widget)
    : LayoutItem(std::move(layout), std::move(widget))
    , m_is_collapsed(false)
{
}

Transform2 FlexboxLayoutItem::get_transform() const
{
    return {};
}

} // namespace signal
