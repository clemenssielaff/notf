#include "core/state.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/widget.hpp"

namespace notf {

const std::string& State::get_name() const
{
    for(auto it = m_state_machine->all_states().cbegin(); it != m_state_machine->all_states().cend(); ++it){
        if(it->second.get() == this){
            return it->first;
        }
    }

    // all states have a name
    assert(false);
    static const std::string error_case;
    return error_case;
}

void State::enter_state(std::shared_ptr<Widget> widget)
{
    for(auto it = m_components.begin(); it != m_components.end(); ++it){
        it->second->_register_widget(widget);
    }
}

void State::leave_state(std::shared_ptr<Widget> widget)
{
    for(auto it = m_components.begin(); it != m_components.end(); ++it){
        it->second->_unregister_widget(widget);
    }
}

void State::set_component(std::shared_ptr<Component> component)
{
    if (!component) {
        log_critical << "Cannot set invalid Component";
        return;
    }
    remove_component(component->get_kind());
    m_components[component->get_kind()] = std::move(component);
}

} // namespace notf
