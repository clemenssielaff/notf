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
    : Item()
    , m_scissor_layout() // empty by default
{
}

bool Widget::_redraw()
{
    // TODO: check for current RenderComponent
    return Item::_redraw();
}

} // namespace notf
