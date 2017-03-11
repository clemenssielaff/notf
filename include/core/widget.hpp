#pragma once

#include <map>

#include "common/exception.hpp"
#include "common/string.hpp"
#include "core/item.hpp"
#include "core/property.hpp"
#include "graphics/cell.hpp"

namespace notf {

class Layout;
class MouseEvent;
class Painter;
class RenderContext;

/**********************************************************************************************************************/

/** A Widget is something drawn on screen that the user can interact with.
 * The term "Widget" is a mixture of "Window" and "Gadget".
 *
 * This class `AbstractWidget` serves as the common baseclass for both C++ and Python Widgets.
 * On the C++ side, you should derive from Widget<WidgetSubclass>, allowing the use of a State machine and Property
 * expressions.
 *
 * Cells
 * =====
 * While the Widget determines the size and the state of what is drawn, the actual drawing is performed on "Cells".
 * A Widget can have multiple Cells.
 *
 * Scissoring
 * ==========
 * In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
 * those Widgets need to be "scissored" by the scroll area.
 * A "Scissor" is an axis-aligned rectangle, scissoring is the act of cutting off parts of a Widget that fall outside
 * that rectangle and since this operation is provided by OpenGL, it is pretty cheap.
 * Every Widget contains a pointer to the parent Layout that acts as its scissor.
 * An empty pointer means that this item is scissored by its parent Layout, but every Layout in this Widget's Item
 * ancestry can be used (including the Windows LayoutRoot, which effectively disables scissoring).
 */
class AbstractWidget : public Item {

protected: // Constructor
    /** Default Constructor. */
    explicit AbstractWidget();

public: // methods
    /** Destructor. */
    virtual ~AbstractWidget() override;

    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> scissor() const { return m_scissor_layout.lock(); }

    /** Sets the new scissor Layout for this Widget.
     * @param scissor               New scissor Layout, must be an Item in this Widget's item hierarchy or empty.
     * @throw std::runtime_error    If the scissor is not an ancestor Item of this Widget.
     */
    void set_scissor(std::shared_ptr<Layout> scissor);

    /** Sets a new Claim for this Widget.
     * @return  True, iff the currenct Claim was changed.
     */
    bool set_claim(const Claim claim);

    /** Tells the Render Manager that this Widget needs to be redrawn. */
    void redraw();

    /** Draws the Widget's Cell onto the screen.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void paint(RenderContext& context) const;

public: // signals
    /** Signal invoked when this Widget is asked to handle a Mouse move event. */
    Signal<MouseEvent&> on_mouse_move;

    /** Signal invoked when this Widget is asked to handle a Mouse button event. */
    Signal<MouseEvent&> on_mouse_button;

private: // methods
    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    virtual void _widgets_at(const Vector2f& local_pos, std::vector<AbstractWidget*>& result) override;

private: // fields
    /** Reference to a Layout used to 'scissor' this item.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;

    /** Cell to draw this Widget into. */
    mutable Cell m_cell;
};

/**********************************************************************************************************************/

/** Widget is a CRTP baseclass for all Widgets implemented in C++.
 * See https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 *
 * State Machine
 * =============
 * Widget subclasses are equipped with a built-in state machine that manages their Properties and Signal connections.
 * Since the State machine has to be provided in the Widget's constructor, it is a good idea to implement a private
 * method `intialize_state_machine`, that returns a fully defined State machine for the subclass.
 */
template <typename WidgetSubclass>
class Widget : public AbstractWidget {

protected: // types
    class State;
    using StateMap   = std::map<std::string, std::unique_ptr<State>>;
    using Transition = std::function<void(WidgetSubclass&)>;

    /******************************************************************************************************************/

    /** A Widget State is a pair of functions (enter and leave) that both take the Widget subclass as mutable argument.
     * This approach seems to be the most general, since the State can not only describe a set of absolute Property
     * values (it can do that as well), but it can also describe a delta to the previous state.
     */
    class State {

    public: // methods
        /** Value Constructor. */
        State(Transition enter, Transition leave, typename StateMap::const_iterator it)
            : m_enter(std::move(enter)), m_leave(std::move(leave)), m_it(std::move(it)) {}

        /** Called when the Widget enters this widget. */
        void enter(WidgetSubclass& widget) const { m_enter(widget); }

        /** Called when the Widget leaves this State. */
        void leave(WidgetSubclass& widget) const { m_leave(widget); }

        /** The name of this State. */
        const std::string& name() const { return m_it->first; }

    private: // fields
        /** Function called when entering the State. */
        const Transition m_enter;

        /** Function called when leaving the State. */
        const Transition m_leave;

        /** Iterator used to reference the name of this State. */
        const typename StateMap::const_iterator m_it;
    };

    /******************************************************************************************************************/

    /** A State Machine is a collection of named States. */
    class StateMachine {

    public: // methods
        /** Default Constructor. */
        StateMachine() = default;

        // no copy / assignment
        StateMachine(const StateMachine&) = delete;
        StateMachine& operator=(const StateMachine&) = delete;

        /** Move Constructor. */
        StateMachine(StateMachine&& other)
            : m_states() { std::swap(m_states, other.m_states); }

        /** Adds a new State to the StateMachine.
         * @return                      The new State.
         * @throw std::runtime_error    If the State could not be added.
         */
        const State* add_state(std::string name, Transition enter, Transition leave)
        {
            if (name.empty()) {
                throw_runtime_error("Cannot add a State without a name to the StateMachine");
            }
            typename StateMap::iterator it;
            bool success;
            std::tie(it, success) = m_states.emplace(std::make_pair(std::move(name), nullptr));
            if (!success) {
                throw_runtime_error(string_format("Cannot replace existing State \"%s\" in StateMachine", name.c_str()));
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
                throw_runtime_error(string_format("Unknown State \"%s\" requested", name.c_str()));
            }
            return it->second.get();
        }

    private: // fields
        /** All States in this StateMachine. */
        StateMap m_states;
    };

    /******************************************************************************************************************/

protected: // methods
    /** Value Constructor.
     * @param state_machine     StateMachine of this Widget.
     * @param properties        All Properties of this Widget.
     */
    Widget(StateMachine&& state_machine, PropertyMap&& properties)
        : AbstractWidget()
        , m_state_machine(std::move(state_machine))
        , m_property_map(std::move(properties))
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
            throw_runtime_error("Cannot transition to null state");
        }
        if (m_current_state) {
            m_current_state->leave(static_cast<WidgetSubclass&>(*this));
        }
        m_current_state = next;
        m_current_state->enter(static_cast<WidgetSubclass&>(*this));
    }

    /** Overload to transition to a new State by name.
     * @param state                 Name of the State to transition to.
     * @throw std::runtime_error    If a State by the given name could not be found.
     */
    void transition_to(const std::string& state) { transition_to(m_state_machine.get_state(state)); }

    /** Returns the name of the current State or an empty string, if the Widget doesn't have a State. */
    const std::string& get_current_state_name() const
    {
        static const std::string empty;
        return m_current_state ? m_current_state->get_name() : empty;
    }

private: // fields
    /** The Widget's StateMachine. */
    const StateMachine m_state_machine;

    /** Map of all the Properties of this Widget. */
    const PropertyMap m_property_map;

    /** State that the Widget is currently in. */
    const State* m_current_state;
};

} // namespace notf

/* Draw / Redraw procedure:
 * A LayoutItem changes a Property
 * This results in the RenderManager getting a request to redraw the frame
 * The RenderManager then collects all Widgets, sorts them calls Widget::paint() on them
 * Widget::paint() checks if it needs to update its Cell
 *  if so, it creates a Painter object and calls the pure virtual function Widget::_paint(Painter&) which does the actual work
 *
 */

#if 0
#include <iostream>
using namespace std;

#include "core/controller.hpp"
using namespace notf;

class Dynamite : public Controller<Dynamite> {
public:
    Dynamite()
        : Controller<Dynamite>(init_state_machine())
    {
        cout << "Starting in State: " << get_current_state_name() << endl;
        transition_to(m_state_calm);
    }

    void go_boom() { transition_to(m_state_boom); }
private:
    const std::string& get_explosive() const
    {
        static const std::string explosive = "dynamite";
        return explosive;
    }

    StateMachine init_state_machine()
    {
        StateMachine state_machine;

        m_state_calm = state_machine.add_state(
            "calm",
            [](Dynamite& self) { cout << "I'm loaded with " << self.get_explosive() << endl; }, // enter
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
