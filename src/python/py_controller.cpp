#include <map>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "core/controller.hpp"
#include "python/docstr.hpp"
#include "python/py_fwd.hpp"
#include "python/type_patches.hpp"
using namespace notf;

/* Trampoline Class ***************************************************************************************************/

class PyController : public AbstractController {

private: // struct
    struct State {
        using Map = std::map<std::string, State>;

        Map::const_iterator it;
        py::weakref enter;
        py::weakref leave;
    };

public: // methods
    PyController()
        : AbstractController()
        , m_states()
        , m_current_state(nullptr)
    {
    }
    // TODO: states, at the moment, cause a unbreakable self-reference loop of PyControllers
    // which means every Controller that calls self.add_state() will never be deleted :(
    // I think I can fix it using either weak references or a pure python implementation instead
    // (there is no real advantage of having a c++ trampoline for something that is easier done in Python anyway)

    virtual ~PyController() override;

    using AbstractController::_set_root_item;

    /** Adds a new State to the Controller's state machine.
     * @return                      The new State.
     * @throw std::runtime_error    If the State could not be added.
     */
    void add_state(std::string name, py::function enter, py::function leave)
    {
        if (name.empty()) {
            std::string msg = "Cannot add a State without a name to the StateMachine";
            log_critical << msg;
            throw std::runtime_error(msg);
        }

        State::Map::iterator it;
        bool success;
        std::tie(it, success) = m_states.emplace(std::make_pair(std::move(name), State()));
        if (!success) {
            std::string msg = string_format("Cannot replace existing State \"%s\" in StateMachine", name.c_str());
            log_critical << msg;
            throw std::runtime_error(msg);
        }
        it->second.it = it;
//        it->second.enter = py::weakref(enter.release()); // doesn't work
//        it->second.leave = py::weakref(leave);
    }

    /** Checks if the Controller has a State with the given name. */
    bool has_state(const std::string& name) const { return m_states.find(name) != m_states.end(); }

    /** Returns the name of the current State or an empty String, if the Controller doesn't have a State. */
    const std::string& get_current_state() const
    {
        static const std::string empty;
        return m_current_state ? m_current_state->it->first : empty;
    }

    /** Changes the current State and executes the relevant leave and enter-functions.
     * @param state                 Name of the State to transition to.
     * @throw std::runtime_error    If the given pointer is the nullptr.
     */
    void transition_to(const std::string& state)
    {
        auto it = m_states.find(state);
        if (it == m_states.end()) {
            std::string msg = string_format("Unknown State \"%s\" requested", state.c_str());
            log_critical << msg;
            throw std::runtime_error(msg);
        }
        State& next = it->second;
        if (m_current_state) {
//            m_current_state->leave();
        }
        m_current_state = &next;
//        m_current_state->enter();
    }

private: // fields
    /** All States of this Controller. */
    State::Map m_states;

    /** The current State of this Controller. */
    State* m_current_state;
};
PyController::~PyController() {}

/* Bindings ***********************************************************************************************************/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_controller(pybind11::module& module, py::detail::generic_type Py_Item)
{
    py::class_<AbstractController, std::shared_ptr<AbstractController>> Py_AbstractController(module, "_AbstractController", Py_Item);
    py::class_<PyController, std::shared_ptr<PyController>> Py_Controller(module, "Controller", Py_AbstractController);
    patch_type(Py_Controller.ptr());

    Py_Controller.def(py::init<>());
    Py_Controller.def("set_root_item", &PyController::_set_root_item, DOCSTR("Sets the LayoutItem at the root of the branch managedby this Controller."));

    Py_Controller.def("get_id", &PyController::get_id, DOCSTR("The application-unique ID of this Controller."));
    Py_Controller.def("has_parent", &PyController::has_parent, DOCSTR("Checks if this Item currently has a parent Item or not."));
    Py_Controller.def("get_current_state", &PyController::get_current_state, DOCSTR("Returns the name of the current State or an empty String, if the Controller doesn't have a State."));

    Py_Controller.def("add_state", &PyController::add_state, DOCSTR("Adds a new state to the Controller's state machine"), py::arg("name"), py::arg("enter_func"), py::arg("leave_func"));
    Py_Controller.def("transition_to", &PyController::transition_to, DOCSTR("Changes the current State and executes the relevant leave and enter-functions."), py::arg("state"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// TODO: finish PyController bindings
