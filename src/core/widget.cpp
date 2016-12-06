#include "core/widget.hpp"

#include <exception>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "core/state.hpp"
#include "utils/make_smart_enabler.hpp"

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

Widget::Widget(std::shared_ptr<StateMachine> state_machine)
    : LayoutItem()
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
