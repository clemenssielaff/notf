#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/layout_item_manager.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"
#include "utils/smart_enabler.hpp"

namespace signal {

std::shared_ptr<Window> Widget::get_window() const
{
    std::shared_ptr<const WindowWidget> window_widget = get_window_widget();
    if (!window_widget) {
        log_critical << "Cannot determine Window for unrooted Widget " << get_handle();
        return {};
    }
    return window_widget->get_window();
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
    // widgets without a Window cannot be drawn
    std::shared_ptr<Window> window = get_window();
    if (!window) {
        return;
    }

    // TODO: proper redraw respecting the FRAMING of each child and dirtying of course
    if (auto internal_child = get_internal_child()) {
        internal_child->redraw();
    }
    for (auto external_child : get_external_children()) {
        external_child->redraw();
    }
    if (has_component_kind(Component::KIND::RENDER)) {
        window->get_render_manager().register_widget(get_handle());
    }
}

std::shared_ptr<Widget> Widget::create(Handle handle)
{
    LayoutItemManager& manager = Application::get_instance().get_layout_item_manager();
    if (!handle) {
        handle = manager.get_next_handle();
    }
    std::shared_ptr<Widget> widget = std::make_shared<MakeSmartEnabler<Widget>>(handle);
    if (!manager.register_item(widget)) {
        log_critical << "Cannot register Widget with handle " << handle
                     << " because the handle is already taken";
        return {};
    }
    log_trace << "Created Widget with handle:" << handle;
    return widget;
}

} // namespace signal
