#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/object_manager.hpp"
#include "core/render_manager.hpp"
#include "core/layout_root.hpp"
#include "core/window.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

std::shared_ptr<Window> Widget::get_window() const
{
    std::shared_ptr<const LayoutRoot> root_item = get_root();
    if (!root_item) {
        log_critical << "Cannot determine Window for unrooted Widget " << get_handle();
        return {};
    }
    return root_item->get_window();
}

void Widget::add_component(std::shared_ptr<Component> component)
{
    if (!component) {
        log_critical << "Cannot add invalid Component to Widget " << get_handle();
        return;
    }
    remove_component(component->get_kind());
    component->register_widget(get_handle());
    m_components[component->get_kind()] = std::move(component);
}

void Widget::remove_component(Component::KIND kind)
{
    if (!has_component_kind(kind)) {
        return;
    }
    auto it = m_components.find(kind);
    assert(it != m_components.end());
    it->second->unregister_widget(get_handle());
    m_components.erase(it);
}

void Widget::redraw()
{
    // don't draw invisible widgets
    if (get_visibility() != VISIBILITY::VISIBLE) {
        return;
    }

    LayoutItem::redraw();

    // if this Widget is renderable, register yourself to be rendered in the next frame
    if (has_component_kind(Component::KIND::RENDER)) {
        std::shared_ptr<Window> window = get_window();
        assert(window);
        window->get_render_manager().register_widget(get_handle());
    }
}

} // namespace notf
