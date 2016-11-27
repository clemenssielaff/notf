#pragma once

#include <memory>
#include <set>
#include <string>

#include "common/enummap.hpp"
#include "core/component.hpp"

namespace notf {

class Widget;

// TODO: should there be a StateMachineFactory that creates a validated and optimized StateMachine
// and would also take care of disallowing all further modifications to the state machine once build?

/**
 * A StateMachine is an object managing a set of State%s.
 * It is owned by Widget%s who share a shared_ptr to it, while it in turn owns the State%s inside.
 * Widget%s keep raw const pointers to their current State.
 */
class StateMachine : public std::enable_shared_from_this<StateMachine> {

public: // methods
    /** Returns a State in this StateMachine by name. */
    const State* get_state(const std::string& name) const
    {
        auto it = m_states.find(name);
        if (it == m_states.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    /** All States of this State machine. */
    const auto& all_states() const { return m_states; }

private: // fields
    /** All states in this State Machine indexed by name. */
    std::unordered_map<std::string, std::unique_ptr<State>> m_states;
};

/**********************************************************************************************************************/

class State {

    friend class StateMachine;

public: // methods
    /** The name of this State.
     * Is potentially expensive, only use for error reporting or similar edge-cases.
     */
    const std::string& get_name() const;

    /** Checks if this Widget contains a Component of the given kind. */
    bool has_component_kind(Component::KIND kind) const { return m_components.count(kind); }

    /** Requests a given kind of Component from this State.
     * @return The requested Component, shared pointer is empty if this State has no Component of the requested kind.
     */
    template <typename COMPONENT>
    std::shared_ptr<COMPONENT> get_component() const
    {
        auto it = m_components.find(get_kind<COMPONENT>());
        if (it == m_components.end()) {
            return {};
        }
        return std::static_pointer_cast<COMPONENT>(it->second);
    }

    /** Registers the Widgets to all of this State's Component%s. */
    void enter_state(std::shared_ptr<Widget> widget);

    /** Unregisters the Widgets from all of this State's Component%s. */
    void leave_state(std::shared_ptr<Widget> widget);

private: // methods for StateMachine
    void add_transition(const State* state) { m_transitions.insert(state); }

    void remove_transition(const State* state) { m_transitions.erase(state); }

    /** Attaches a new Component to this State.
     * A State can only hold one instance of each Component kind.
     */
    void set_component(std::shared_ptr<Component> component);

    /** Removes the Component of the given kind from this State.
     * If the State doesn't have the given Component kind, the call is ignored.
     */
    void remove_component(Component::KIND kind) { m_components.erase(kind); }

private: // fields
    /** The StateMachine that this State is a part of. */
    const StateMachine* m_state_machine;

    /** States to which you can transition from this one. */
    std::set<const State*> m_transitions;

    /** All Components of this State. */
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
};

} // namespace notf
