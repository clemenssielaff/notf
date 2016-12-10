#pragma once

#include <exception>
#include <functional>
#include <map>
#include <memory>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "core/layout_item.hpp"

namespace notf {

/**********************************************************************************************************************/

/** Item interface to a Controller instace.
 * C++ subclasses should inherit from Controller<T> instead.
 * Is also used as baseclass for Python Controllers.
 */
class AbstractController : public Item {

public: // methods
    virtual ~AbstractController() override;

    /** Returns the root item managed by this Controller. */
    const Item* get_root_item() const { return m_root_item.get(); }

    virtual float get_opacity() const override { return m_root_item->get_opacity(); }

    virtual const Size2f& get_size() const override { return m_root_item->get_size(); }

    virtual const Claim& get_claim() const override { return m_root_item->get_claim(); }

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override
    {
        return m_root_item->get_widget_at(local_pos);
    }

protected: // methods
    explicit AbstractController(std::shared_ptr<LayoutItem> root_item)
        : Item()
        , m_root_item(std::move(root_item))
    {
    }

    virtual bool _set_opacity(float opacity) override { return m_root_item->set_opacity(opacity); }

    virtual bool _set_size(const Size2f size) override { return _set_item_size(m_root_item.get(), std::move(size)); }

    virtual bool _set_transform(const Transform2 transform) override
    {
        return _set_item_transform(m_root_item.get(), std::move(transform));
    }

private: // methods
    virtual Transform2 _get_local_transform() const override { return m_root_item->get_transform(Space::LOCAL); }

private: // fields
    /** Item at the root of the Controller's Item hierarchy. */
    std::shared_ptr<LayoutItem> m_root_item;
};

/**********************************************************************************************************************/

/**
 * Controller baseclass, use a curiously recurring template.
 */
template <typename T>
class Controller : public AbstractController {

protected:
    class State;
    using Transition = std::function<void(T&)>;
    using StateMap = std::map<std::string, std::unique_ptr<State>>;

    /******************************************************************************************************************/

    class StateMachine {
    public: // methods
        StateMachine() = default;

        // no copy / assignment
        StateMachine(const StateMachine&) = delete;
        StateMachine& operator=(const StateMachine&) = delete;

        /** Move Constructor. */
        StateMachine(StateMachine&& other)
            : m_states()
        {
            std::swap(m_states, other.m_states);
        }

        /** Adds a new State to the StateMachine.
         * @return                      The new State.
         * @throw std::runtime_error    If the State could not be added.
         */
        const State* add_state(std::string name, Transition enter, Transition leave)
        {
            if (name.empty()) {
                std::string msg = "Cannot add a State without a name to the StateMachine";
                log_critical << msg;
                throw std::runtime_error(msg);
            }

            typename StateMap::iterator it;
            bool success;
            std::tie(it, success) = m_states.emplace(std::make_pair(std::move(name), nullptr));
            if (!success) {
                std::string msg = string_format("Cannot replace existing State \"%s\" in StateMachine", name.c_str());
                log_critical << msg;
                throw std::runtime_error(msg);
            }
            it->second.reset(new State(enter, leave, it));
            return it->second.get();
        }

        /** Checks if the StateMachine has a State with the given name. */
        bool has_state(const std::string& name) const { return m_states.find(name) != m_states.end(); }

        /** Returns a State by name.
         * @return                      The new requested State.
         * @throw std::runtime_error    If the State could not be found.
         */
        const State* get_state(const std::string& name) const
        {
            auto it = m_states.find(name);
            if (it == m_states.end()) {
                std::string msg = string_format("Unknown State \"%s\" requested", name.c_str());
                log_critical << msg;
                throw std::runtime_error(msg);
            }
            return it->second.get();
        }

    private: // fields
        /** All States in this StateMachine. */
        StateMap m_states;
    };

    /******************************************************************************************************************/

    class State {
        friend class StateMachine;

    private: // methods
        State(Transition enter, Transition leave, typename StateMap::const_iterator it)
            : m_enter(std::move(enter))
            , m_leave(std::move(leave))
            , m_it(std::move(it))
        {
        }

    public: // methods
        /** Called when the Controller enters this State. */
        void enter(T& controller) { m_enter(controller); }

        /** Called when the Controller leaves this State. */
        void leave(T& controller) { m_leave(controller); }

        /** The name of this State. */
        const std::string& get_name() const { return m_it->first; } // const so Controllers can use it

    private: // fields
        /** Function called when entering the State. */
        const Transition m_enter;

        /** Function called when leaving the State. */
        const Transition m_leave;

        /** Iterator used to reference the name of this State. */
        const typename StateMap::const_iterator m_it;
    };

    /******************************************************************************************************************/

protected: // methods
    /**
     * @param root_item     Root item that this Controller manages.
     * @param state_machine StateMachine of this Controller, can only be created by the subclass.
     */
    Controller(std::shared_ptr<LayoutItem> root_item, StateMachine&& state_machine)
        : AbstractController(std::move(root_item))
        , m_state_machine(std::move(state_machine))
        , m_current_state(nullptr)
    {
    }

    /** Changes the current State and executes the relevant leave and enter-functions.
     * @param next                  State to transition into.
     * @throw std::runtime_error    If the given pointer is the nullptr.
     */
    void transition_to(const State* next)
    {
        if (!next) {
            std::string msg = "Cannot transition to null state";
            log_critical << msg;
            throw std::runtime_error(msg);
        }
        if (m_current_state) {
            m_current_state->leave(static_cast<T&>(*this));
        }
        m_current_state = const_cast<State*>(next); // Controller subclasses may only have const States, we need mutable
        m_current_state->enter(static_cast<T&>(*this));
    }
    void transition_to(const std::string& state) { transition_to(m_state_machine.get_state(state)); }

    /** Returns the name of the current State or an empty String, if the Controller doesn't have a State. */
    const std::string& get_current_state_name() const
    {
        static const std::string empty;
        return m_current_state ? m_current_state->get_name() : empty;
    }

private: // fields
    /** The Controller's StateMachine. */
    const StateMachine m_state_machine;

    /** State that the Controller is currently in. */
    State* m_current_state;
};

} // namespace notf
