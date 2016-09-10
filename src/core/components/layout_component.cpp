#include "core/components/layout_component.hpp"

#include "common/devel.hpp"
#include "common/log.hpp"
#include "core/widget.hpp"

namespace signal {

LayoutComponent::LayoutComponent()
    : m_layouts()
    , m_internal_layout()
{
}

template <typename LAYOUT, typename>
std::shared_ptr<LAYOUT> LayoutComponent::create_layout(const std::string& name)
{
    if (m_layouts.count(name)) {
        log_critical << "Could not add layout '" << name
                     << "'to LayoutComponent, because it already contains another layout by the same name.";
        return {};
    }
    std::shared_ptr<LAYOUT> layout = std::make_shared<MakeSharedEnabler<LAYOUT>>(this, name);
    m_layouts.emplace(std::make_pair(name, layout));
    return layout;
}

void LayoutComponent::set_internal_layout(std::shared_ptr<Layout> layout)
{
    if (layout->get_layout_component() != this) {
        log_critical << "Could not set internal layout '" << layout->get_name()
                     << "'because it is not owned by this LayoutComponent.";
        return;
    }
    assert(m_layouts.count(layout->get_name()));
    assert(m_layouts.at(layout->get_name()) == layout);

    if (layout) {
        m_internal_layout = layout;
    }
    else {
        m_internal_layout.reset();
    }
}

LayoutItem::~LayoutItem()
{
}

Layout::~Layout()
{
}

} // namespace signal
