/// Controllers in NoTF.
/// There are 3 main types in NoTF to construct a user interface: Widgets, Layouts and Controllers.
/// * Widgets are anything that directly interact with the user: anything you can see on the screen and click on.
/// * Layouts are a nested structure of rectangles that position the Widgets and give them an appropriate size.
/// * Controllers are the manager of both Layouts and Widgets and create, modify and destroy them as necessary.
#pragma once

#include <map>
#include <sstream>

#include "app/core/property_graph.hpp"
#include "app/scene/item.hpp"
#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

class ScreenItem;

//====================================================================================================================//

/// A Controller managing Layouts and Widgets.
/// This class `Controller` serves as the common baseclass for both C++ and Python Controller.
/// On the C++ side, you should derive from BaseController<ControllerSubclass>, allowing the use of a State machine and
/// Property expressions.
class Controller : public Item {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token        Factory token provided by Item::_create.
    Controller(const Token& token);

public:
    /// Item at the root of the Controller's branch of the Item hierarchy.
    ScreenItem* root_item() { return m_root_item; }
    const ScreenItem* root_item() const { return const_cast<Controller*>(this)->root_item(); }

    /// Initializes this Controller if it is uninitialized, otherwise does nothing.
    void initialize();

protected:
    /// Sets a new root at this Controller's branch of the Item hierarchy.
    void _set_root_item(const ScreenItemPtr& item);

    virtual void _remove_child(const Item* child_item) override;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Item at the root of the Controller's Item hierarchy.
    ScreenItem* m_root_item;
};

//====================================================================================================================//

/// BaseController is a CRTP baseclass for all Controller implemented in C++.
/// See https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
///
/// State Machine
/// =============
/// BaseController subclasses are equipped with a built-in state machine that manages their Properties and Signal
/// connections.
/// Since the State machine has to be provided in the BaseController's constructor, it is a good idea to implement a
/// private method `intialize_state_machine`, that returns a fully defined State machine for the subclass.
template<typename ControllerSubclass>
class BaseController : public Controller {

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    class State;
    using StateMap   = std::map<std::string, std::unique_ptr<State>>;
    using Transition = std::function<void(ControllerSubclass&)>;

    //=======================================================================//

    /// A Controller State is a pair of functions (enter and leave) that both take the instance as mutable argument.
    /// This approach seems to be the most general, since the State can not only describe a set of absolute Property
    /// values (it can do that as well), but it can also describe a delta to the previous state.
    class State {

    public: // methods
        /// Value Constructor.
        State(Transition enter, Transition leave, typename StateMap::const_iterator it)
            : m_enter(std::move(enter)), m_leave(std::move(leave)), m_it(std::move(it))
        {}

        /// Called when the Controller enters this State.
        void enter(ControllerSubclass& controller) const { m_enter(controller); }

        /// Called when the Controller leaves this State.
        void leave(ControllerSubclass& controller) const { m_leave(controller); }

        /// The name of this State.
        const std::string& name() const { return m_it->first; }

    private: // fields
        /// Function called when entering the State.
        const Transition m_enter;

        /// Function called when leaving the State.
        const Transition m_leave;

        /// Iterator used to reference the name of this State.
        const typename StateMap::const_iterator m_it;
    };

    //=======================================================================//

    /// A State Machine is a collection of named States.
    class StateMachine {

    public: // methods
        NOTF_NO_COPY_OR_ASSIGN(StateMachine)

        /// Default Constructor.
        StateMachine() = default;

        /// Move Constructor.
        StateMachine(StateMachine&& other) : m_states() { std::swap(m_states, other.m_states); }

        /// Adds a new State to the StateMachine.
        /// @return                 The new State.
        /// @throws runtime_error   If the State could not be added.
        const State* add_state(std::string name, Transition enter, Transition leave)
        {
            if (name.empty()) {
                notf_throw(runtime_error, "Cannot add a State without a name to the StateMachine");
            }
            typename StateMap::iterator it;
            bool success;
            std::tie(it, success) = m_states.emplace(std::make_pair(std::move(name), nullptr));
            if (!success) {
                notf_throw_format(runtime_error, "Cannot replace existing State \"" << name << "\" in StateMachine");
            }
            it->second.reset(new State(enter, leave, it));
            return it->second.get();
        }

        /// Checks if the StateMachine has a State with the given name.
        bool has_state(const std::string& name) const { return m_states.find(name) != m_states.end(); }

        /// Returns a State by name.
        /// @return                      The new requested State.
        /// @throw std::runtime_error    If the State could not be found.
        const State* state(const std::string& name) const
        {
            auto it = m_states.find(name);
            if (it == m_states.end()) {
                notf_throw_format(runtime_error, "Unknown State \"" << name << "\" requested");
            }
            return it->second.get();
        }

    private: // fields
        /// All States in this StateMachine.
        StateMap m_states;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Value Constructor.
    /// @param token            Factory token provided by Item::_create.
    /// @param state_machine     StateMachine of this Controller.
    /// @param properties        All Properties of this Controller.
    BaseController(const Token& token, StateMachine&& state_machine,
                   std::unordered_map<std::string, PropertyId>&& properties)
        : Controller(token)
        , m_state_machine(std::move(state_machine))
        , m_property_map(std::move(properties))
        , m_current_state(nullptr)
    {}

    /// Changes the current State and executes the relevant leave and enter-functions.
    /// @param next                  State to transition into.
    /// @throw std::runtime_error    If the given pointer is the nullptr.
    void transition_to(const State* next)
    {
        if (!next) {
            notf_throw(runtime_error, "Cannot transition to null state");
        }
        if (m_current_state) {
            m_current_state->leave(static_cast<ControllerSubclass&>(*this));
        }
        m_current_state = next;
        m_current_state->enter(static_cast<ControllerSubclass&>(*this));
    }

    /// Overload to transition to a new State by name.
    /// @param state                 Name of the State to transition to.
    /// @throw std::runtime_error    If a State by the given name could not be found.
    void transition_to(const std::string& state) { transition_to(m_state_machine.state(state)); }

    /// Returns the name of the current State or an empty string, if the Controller doesn't have a State.
    const std::string& current_state_name() const
    {
        static const std::string empty;
        return m_current_state ? m_current_state->name() : empty;
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The Controller's StateMachine.
    const StateMachine m_state_machine;

    /// Map of all the Properties of this Controller.
    const std::unordered_map<std::string, PropertyId> m_property_map;

    /// State that the Controller is currently in.
    const State* m_current_state;
};

NOTF_CLOSE_NAMESPACE

#if 0
#    include <iostream>
using namespace std;

#    include "core/controller.hpp"
NOTF_USING_NAMESPACE

class Dynamite : public Controller<Dynamite> {
public:
    Dynamite()
        : Controller<Dynamite>(init_state_machine())
    {
        cout << "Starting in State: " << current_state_name() << endl;
        transition_to(m_state_calm);
    }

    void go_boom() { transition_to(m_state_boom); }
private:
    const std::string& explosive() const
    {
        static const std::string explosive = "dynamite";
        return explosive;
    }

    StateMachine init_state_machine()
    {
        StateMachine state_machine;

        m_state_calm = state_machine.add_state(
            "calm",
            [](Dynamite& self) { cout << "I'm loaded with " << self.explosive() << endl; }, // enter
            [](Dynamite&) { cout << "Tick tick tick..." << endl; }); // leave

        m_state_boom = state_machine.add_state(
            "kaboom",
            [](Dynamite&) { cout << "Kaboom" << endl; }, // enter
            {}); // leave

        return state_machine;
    }

private:
    const State* m_state_calm;
    const State* m_state_boom;
};

int main()
//int notmain()
{
    Dynamite stick;
    stick.go_boom();
    return 0;
}

#endif
