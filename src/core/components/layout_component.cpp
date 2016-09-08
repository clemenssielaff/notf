#include "core/components/layout_component.hpp"

namespace signal {

LayoutComponent::LayoutComponent(std::shared_ptr<Widget> owner)
    : m_widget(owner)
{
}

LayoutItem::LayoutItem(std::shared_ptr<LayoutComponent> layout, std::shared_ptr<Widget> widget)
    : m_layout(layout)
    , m_widget(widget)
{
}

} // namespace signal
