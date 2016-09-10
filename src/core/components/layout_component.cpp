#include "core/components/layout_component.hpp"

#include "core/widget.hpp"

namespace signal {

LayoutComponent::LayoutComponent(std::shared_ptr<Widget> owner)
    : m_widget(owner)
{
}

LayoutItem::LayoutItem(std::shared_ptr<LayoutComponent> layout, std::shared_ptr<Widget> widget)
    : m_layout(layout)
    , m_widget(widget)
{
    widget->set_layout_item(shared_from_this());
}

} // namespace signal
