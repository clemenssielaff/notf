#pragma once

#include <memory>
#include <set>
#include <string>

#include "common/enummap.hpp"
#include "core/component.hpp"

namespace notf {

class Widget;
class StateMachine;

/**********************************************************************************************************************/

/**
 * StateMachine%s and State%s are immutable to simplify their interface and ensure stability at runtime.
 * All mutability, like adding / removing State%s and Transitions happens during the construction phase, within the
 * StateMachineFactory.
 */
class StateMachineFactory {

public: // class
    /** Helper struct acting as a blueprint to build a State from. */
    class StateStudy {

        friend class StateMachineFactory;

    public: // methods
        /** Adds a Transition to another state. */
        void transition_to(std::shared_ptr<StateStudy> state);

        /** Removes the Transition to another state, if there is one. */
        void remove_transition_to(std::shared_ptr<StateStudy> state);

        /** Removes all Transitions out of this state. */
        void remove_all_transitions() { m_transitions.clear(); }

        /** Attaches a new Component to this state, replaces an old Component of the same kind. */
        void attach_component(std::shared_ptr<Component> component);

        /** Removes a certain Component from this state. */
        void remove_component(std::shared_ptr<Component> component);

        /** Removes the Component of a certain type from this state. */
        void remove_component(const Component::KIND kind);

        /** Removes all Components from this state. */
        void remove_all_components() { m_components.clear(); }

    protected: // methods
        /** @param name  Name of this state, cannot be changed. */
        explicit StateStudy(const std::string& name)
            : m_name(name)
            , m_transitions()
            , m_components()
        {
        }

    private: // fields
        /** The name of this state. */
        std::string m_name;

        /** All Transitions out of this state. */
        std::set<std::shared_ptr<StateStudy>> m_transitions;

        /** All Components of this state. */
        EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
    };

public: // methods
    StateMachineFactory() = default;

    /** Creates a new state with the given name.
     * @return The new state. If another state by the same name already exists, this function returns empty.
     */
    std::shared_ptr<StateStudy> add_state(const std::string& name);

    /** Returns an existing state by name, returns empty if the name is unknown. */
    std::shared_ptr<StateStudy> get_state(const std::string& name);

    /** Removes all Transitions into a given state. */
    void remove_all_transitions_to(std::shared_ptr<StateStudy> state);

    /** Produces a valid StateMachine instance.
     * @param start_state   The start state of the state machine.
     */
    std::shared_ptr<StateMachine> produce(std::shared_ptr<StateStudy> start_state);

private: // fields
    std::unordered_map<std::string, std::shared_ptr<StateStudy>> m_states;
};

/**********************************************************************************************************************/

/**
 * A StateMachine is an object managing a set of State%s.
 * It is owned by Widget%s who share a shared_ptr to it, while it in turn owns the State%s inside.
 * Widget%s keep raw const pointers to their current State.
 */
class StateMachine : public std::enable_shared_from_this<StateMachine> {

    friend class StateMachineFactory;

public: // methods
    /** Returns the start state of this StateMachine. */
    const State* get_start_state() const { return m_start_state; }

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

protected: // methods
    StateMachine() = default;

private: // fields
    /** All states in this State Machine indexed by name. */
    std::unordered_map<std::string, std::unique_ptr<State>> m_states;

    /** The start State of this StateMachine. */
    State* m_start_state;
};

/**********************************************************************************************************************/

/**
 * A State holds Components that define the Widgets in this State.
 */
class State {

    friend class StateMachineFactory;

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

protected: // methods for StateMachineFactory
    /**
     * @param state_machine The StateMachine that this State is a part of.
     * @param components    All Components of this State.
     */
    State(const StateMachine* state_machine, EnumMap<Component::KIND, std::shared_ptr<Component>> components)
        : m_state_machine(state_machine)
        , m_transitions()
        , m_components(std::move(components))
    {
    }

private: // fields
    /** The StateMachine that this State is a part of. */
    const StateMachine* m_state_machine;

    /** States to which you can transition from this one. */
    std::set<const State*> m_transitions;

    /** All Components of this State. */
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
};

} // namespace notf
