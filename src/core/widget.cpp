#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/application.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"
#include "utils/smart_enabler.hpp"

namespace signal {

Widget::~Widget()
{
    about_to_be_deleted();
    log_trace << "Destroyed Widget with handle:" << get_handle();
}

std::shared_ptr<Window> Widget::get_window() const
{
    Application& app = Application::get_instance();
    Handle root_handle = app.get_root(get_handle());
    if(!root_handle){
        log_critical << "Cannot determine Window for unrooted Widget " << get_handle();
        return {};
    }

    std::shared_ptr<LayoutItem> root_item = app.get_item(root_handle);
    std::shared_ptr<WindowWidget> root_widget = std::dynamic_pointer_cast<WindowWidget>(root_item);
    if(!root_widget){
        log_critical << "Expected Widget " << root_item->get_handle() << " to be a WindowWidget but it isn't";
        return {};
    }

    return root_widget->get_window();
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
    Application& app = Application::get_instance();
    if (!handle) {
        handle = app.get_next_handle();
    }
    std::shared_ptr<Widget> widget = std::make_shared<MakeSmartEnabler<Widget>>(handle);
    if (!register_item(widget)) {
        log_critical << "Cannot register Widget with handle " << handle
                     << " because the handle is already taken";
        return {};
    }
    log_trace << "Created Widget with handle:" << handle;
    return widget;
}

} // namespace signal
