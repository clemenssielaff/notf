#include "core/widget.hpp"

#include <exception>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "core/state.hpp"

namespace notf {

const Claim& Widget::get_claim() const
{
    return m_current_state->get_claim();
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

AbstractProperty* Widget::get_property(const std::string& name)
{
    auto it = m_properties.find(name);
    if (it == m_properties.end()) {
        log_warning << "Requested unknown Property \"" << name << "\"";
        return nullptr;
    }
    return it->second.get();
}

std::shared_ptr<Widget> Widget::create(std::shared_ptr<StateMachine> state_machine, Handle handle)
{
    if (std::shared_ptr<Widget> result = _create_object<Widget>(handle, std::move(state_machine))) {
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

Widget::Widget(const Handle handle, std::shared_ptr<StateMachine> state_machine)
    : LayoutItem(handle)
    , m_state_machine(std::move(state_machine))
    , m_current_state(m_state_machine->get_start_state())
    , m_properties()
    , m_scissor_layout() // empty by default
{
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
