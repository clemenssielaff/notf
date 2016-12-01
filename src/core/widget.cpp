#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "core/state.hpp"

namespace notf {

const State* Widget::get_state() const
{
    if (!m_current_state) {
        log_warning << "Requested invalid state for Widget " << get_handle();
    }
    return m_current_state;
}

void Widget::set_state_machine(std::shared_ptr<StateMachine> state_machine)
{
    m_state_machine = state_machine;
    m_current_state = m_state_machine->get_start_state();
    _update_parent_layout();
}

std::shared_ptr<Widget> Widget::get_widget_at(const Vector2& /*local_pos*/)
{
    // if this Widget has no shape, you cannot find it at any location
    if (m_current_state->has_component_kind(Component::KIND::SHAPE)) {
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

bool Widget::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!m_current_state->has_component_kind(Component::KIND::CANVAS)) {
        return false;
    }

    return LayoutItem::_redraw();
}

} // namespace notf
