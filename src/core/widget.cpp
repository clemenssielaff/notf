#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

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
    component->_register_widget(get_handle());
    m_components[component->get_kind()] = std::move(component);
}

void Widget::remove_component(Component::KIND kind)
{
    if (!has_component_kind(kind)) {
        return;
    }
    auto it = m_components.find(kind);
    assert(it != m_components.end());
    it->second->_unregister_widget(get_handle());
    m_components.erase(it);
}

std::shared_ptr<Widget> Widget::get_widget_at(const Vector2& /*local_pos*/)
{
    // if this Widget has no shape, you cannot find it at any location
    if (!has_component_kind(Component::KIND::SHAPE)) {
        return {};
    }

    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    return std::static_pointer_cast<Widget>(shared_from_this());
}

void Widget::_redraw()
{
    // don't draw invisible widgets
    if (get_visibility() != VISIBILITY::VISIBLE) {
        return;
    }

    // if this Widget is renderable, register yourself to be rendered in the next frame
    if (has_component_kind(Component::KIND::RENDER)) {
        std::shared_ptr<Window> window = get_window();
        assert(window);
        window->get_render_manager().register_widget(get_handle());
    }
}

} // namespace notf
