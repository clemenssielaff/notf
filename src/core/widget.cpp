#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

#if SIGNAL_LOG_LEVEL <= 3 // SIGNAL_LOG_LEVEL_WARNING
const State* Widget::get_current_state() const
{
    if(!m_current_state){
        log_warning << "Requested invalid state for Widget " << get_handle();
    }
    return m_current_state;
}
#endif

std::shared_ptr<Window> Widget::get_window() const
{
    std::shared_ptr<const LayoutRoot> root_item = get_root();
    if (!root_item) {
        log_critical << "Cannot determine Window for unrooted Widget " << get_handle();
        return {};
    }
    return root_item->get_window();
}

std::shared_ptr<Widget> Widget::get_widget_at(const Vector2& /*local_pos*/)
{
    // if this Widget has no shape, you cannot find it at any location
    if(m_current_state->has_component_kind(Component::KIND::SHAPE)){
        return {};
    }

    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    return std::static_pointer_cast<Widget>(shared_from_this());
}

std::shared_ptr<Widget> Widget::create(Handle handle)
{
    if (std::shared_ptr<Widget> result = _create_object<Widget>(handle)) {
        return result;
    }

    std::string message;
    if (handle) {
        message = string_format("Failed to create Widget with requested Handle %u", handle);
    }
    else {
        message = "Failed to allocate new Handle for Widget";
    }
    throw std::runtime_error(message);
}

void Widget::_redraw()
{
    // Widgets without a canvas can never be drawn.
    if(m_current_state->has_component_kind(Component::KIND::CANVAS)){
        return;
    }

    std::shared_ptr<Window> window = get_window();
    assert(window);
    RenderManager& render_manager = window->get_render_manager();

    // register yourself to be rendered in the next frame
    if (get_visibility() == VISIBILITY::VISIBLE) {
        render_manager.register_widget(get_handle());
    }
    // ... or unregister, if you have become invisible
    else {
        render_manager.unregister_widget(get_handle());
    }
}

} // namespace notf
