#include "core/state.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/widget.hpp"
#include "utils/enum_to_number.hpp"
#include "utils/make_smart_enabler.hpp"

namespace { // anonymous

const std::string empty_string;

} // namespace anonymous

namespace notf {

void StateMachineFactory::StateStudy::transition_to(std::shared_ptr<StateMachineFactory::StateStudy> state)
{
    if (state.get() == this) {
        log_warning << "States cannot transition to themselves (requested for " << m_name << ")";
        return;
    }
    if (m_transitions.count(state)) {
        log_warning << m_name << " already transitions into " << state->m_name;
        return;
    }
    m_transitions.insert(state);
}

void StateMachineFactory::StateStudy::remove_transition_to(std::shared_ptr<StateMachineFactory::StateStudy> state)
{
    if (!m_transitions.count(state)) {
        log_warning << "Ignoring call to remove unknown transition from State " << m_name;
        return;
    }
    m_transitions.erase(state);
}

void StateMachineFactory::StateStudy::attach_component(std::shared_ptr<Component> component)
{
    if (m_components.count(component->get_kind())) {
        log_warning << "Replacing Component of kind `" << Component::get_kind_name(component->get_kind())
                    << "` in State " << m_name;
    }
    m_components[component->get_kind()] = component;
}

void StateMachineFactory::StateStudy::remove_component(std::shared_ptr<Component> component)
{
    if (!m_components.count(component->get_kind()) || m_components[component->get_kind()] != component) {
        log_warning << "Cannot remove foreign Component of kind `" << Component::get_kind_name(component->get_kind())
                    << "` from State " << m_name;
        return;
    }
    m_components.erase(component->get_kind());
}

void StateMachineFactory::StateStudy::remove_component(const Component::KIND kind)
{
    m_components.erase(kind);
}

std::shared_ptr<StateMachineFactory::StateStudy> StateMachineFactory::add_state(const std::string& name)
{
    if (m_states.count(name)) {
        log_warning << "The StateMachineFactory already contains a State named '"
                    << name << "', returning empty instead";
        return {};
    }
    std::shared_ptr<StateStudy> study = std::make_shared<MakeSmartEnabler<StateMachineFactory::StateStudy>>(name);
    m_states.insert(std::make_pair(name, study));
    return study;
}

std::shared_ptr<StateMachineFactory::StateStudy> StateMachineFactory::get_state(const std::string& name)
{
    if (!m_states.count(name)) {
        log_warning << "Requested unkown State'" << name << "', returning empty instead";
        return {};
    }
    return m_states[name];
}

void StateMachineFactory::remove_all_transitions_to(std::shared_ptr<StateMachineFactory::StateStudy> state)
{
    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        it->second->m_transitions.erase(state);
    }
}

std::shared_ptr<StateMachine> StateMachineFactory::produce(std::shared_ptr<StateMachineFactory::StateStudy> start_state)
{
    // make sure that `start_state` is an actual state in the machine
    if (!m_states.count(start_state->m_name) || m_states[start_state->m_name] != start_state) {
        log_critical << "Failed to produce a StateMachine with a foreign start State '" << start_state->m_name << "'";
        return {};
    }

    // sort out all unreachable states
    std::set<StateStudy*> reachable_states = {start_state.get()};
    {
        std::vector<StateStudy*> reachables = {start_state.get()};
        reachables.reserve(m_states.size()); // to avoid dynamic re-allocation which would invalidate the iterator
        for (auto it = reachables.begin(); it != reachables.end(); ++it) {
            for (const std::shared_ptr<StateStudy>& state : (*it)->m_transitions) {
                if (!reachable_states.count(state.get())) {
                    reachables.push_back(state.get());
                    reachable_states.insert(state.get());
                }
            }
        }
        for (auto it = m_states.cbegin(); it != m_states.cend(); ++it) {
            if (!reachable_states.count(it->second.get())) {
                log_warning << "Ignoring unreachable State \"" << it->second->m_name << "\"";
            }
        }
    }

    // create the state machine and its states without transitions
    std::shared_ptr<StateMachine> state_machine = std::make_shared<MakeSmartEnabler<StateMachine>>();
    for (StateStudy* state : reachable_states) {
        state_machine->m_states[state->m_name] = std::make_unique<MakeSmartEnabler<State>>(state_machine.get(),
                                                                                           state->m_claim,
                                                                                           state->m_components);
    }
    state_machine->m_start_state = state_machine->m_states[start_state->m_name].get();

    // define all transitions next
    for (auto it = state_machine->m_states.begin(); it != state_machine->m_states.end(); ++it) {
        for (const auto& study : m_states[it->first]->m_transitions) {
            it->second.get()->m_transitions.insert(state_machine->m_states[study->m_name].get());
        }
    }

    return state_machine;
}

const std::string& State::get_name() const
{
    for (auto it = m_state_machine->all_states().cbegin(); it != m_state_machine->all_states().cend(); ++it) {
        if (it->second.get() == this) {
            return it->first;
        }
    }

    // all states have a name or something has gone seriously wrong
    log_critical << "Encountered unnamed State";
    assert(false);
    return empty_string;
}

void State::enter_state(std::shared_ptr<Widget> widget)
{
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        it->second->_register_widget(widget);
    }
}

void State::leave_state(std::shared_ptr<Widget> widget)
{
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        it->second->_unregister_widget(widget);
    }
}

} // namespace notf
