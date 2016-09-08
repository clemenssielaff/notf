#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/application.hpp"
#include "core/window.hpp"

namespace signal {

Widget::~Widget()
{
    log_trace << "Destroyed Widget with handle:" << m_handle;
}

void Widget::set_parent(std::shared_ptr<Widget> parent)
{
    std::shared_ptr<Widget> this_widget = shared_from_this();

    // remove yourself from your current parent
    if (std::shared_ptr<Widget> current_parent = m_parent.lock()) {
        bool success = remove_one_unordered(current_parent->m_children, this_widget);
        assert(success);
    }

    // register with the new parent
    m_parent = parent;
    m_window = parent->m_window;
    parent->m_children.emplace_back(std::move(this_widget));
}

void Widget::add_component(std::shared_ptr<Component> component)
{
    if (!component) {
        log_critical << "Cannot add invalid Component to Widget " << m_handle;
        return;
    }
    remove_component(component->get_kind());
    component->register_widget(shared_from_this());
    m_components[component->get_kind()] = std::move(component);
}

void Widget::remove_component(Component::KIND kind)
{
    if(!has_component_kind(kind)){
        return;
    }
    auto it = m_components.find(kind);
    assert(it != m_components.end());
    it->second->unregister_widget(shared_from_this());
    m_components.erase(it);
}

void Widget::redraw()
{
    // widgets without a Window cannot be drawn
    if (!m_window) {
        return;
    }

    // TODO: proper redraw respecting the FRAMING of each child and dirtying of course
    for (const std::shared_ptr<Widget>& child : m_children) {
        child->redraw();
    }
    if(has_component_kind(Component::KIND::RENDER)){
        m_window->get_render_manager().register_widget(shared_from_this());
    }
}

std::shared_ptr<Widget> Widget::make_widget(Handle handle)
{
    Application& app = Application::get_instance();
    if (!handle) {
        handle = app.get_next_handle();
    }
    std::shared_ptr<Widget> widget = std::make_shared<MakeSharedEnabler<Widget>>(handle);
    if (!register_widget(widget)) {
        log_critical << "Cannot register Widget with handle " << handle
                     << " because the handle is already taken";
        return {};
    }
    log_trace << "Created Widget with handle:" << handle;
    return widget;
}

} // namespace signal
