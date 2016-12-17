#include <map>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "core/controller.hpp"
#include "python/docstr.hpp"
#include "python/py_fwd.hpp"
#include "python/type_patches.hpp"
#include "python/py_signal.hpp"
using namespace notf;

/* Trampoline Class ***************************************************************************************************/

class PyController : public AbstractController {

private: // struct
    struct State {
        using Map = std::map<std::string, State>;

        Map::const_iterator it;
        std::unique_ptr<PyObject, decltype(&py_decref)> enter = {nullptr, py_decref};
        std::unique_ptr<PyObject, decltype(&py_decref)> leave = {nullptr, py_decref};
    };

public: // methods
    PyController()
        : AbstractController()
        , m_states()
        , m_current_state(nullptr)
        , py_mouse_event("on_mouse_event")
    {
        connect_signal(on_mouse_event, [this](){py_mouse_event.fire();});
    }

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

        // create an empty state at first and store an iterator into itself for it to access its name
        State::Map::iterator it;
        {
            bool success;
            std::tie(it, success) = m_states.emplace(std::make_pair(std::move(name), State()));
            if (!success) {
                std::string msg = string_format("Cannot replace existing State \"%s\" in StateMachine", name.c_str());
                log_critical << msg;
                throw std::runtime_error(msg);
            }
            it->second.it = it;
        }

        // TODO: if storing python objects into self becomes a pattern, make a function out of it
        { // store the two methods into a cache in the object's __dict__ so they don't get lost
            static const char* cache_name = "__notf_cache";
            int success = 0;
            py::object self = py::cast(this);
            py::object dict(PyObject_GenericGetDict(self.ptr(), nullptr), false);
            py::object cache(PyDict_GetItemString(dict.ptr(), cache_name), true);
            if (!cache.check()) {
                py::object new_cache(PySet_New(nullptr), false);
                success += PyDict_SetItemString(dict.ptr(), cache_name, new_cache.ptr());
                assert(success == 0);
                cache = std::move(new_cache);
            }
            assert(cache.check());
            success += PySet_Add(cache.ptr(), enter.ptr());
            success += PySet_Add(cache.ptr(), leave.ptr());
            assert(success == 0);
        }

        // ... and only keep weakrefs yourself, otherwise they will keep this object alive forever
        it->second.enter.reset(PyWeakref_NewRef(enter.ptr(), nullptr));
        it->second.leave.reset(PyWeakref_NewRef(leave.ptr(), nullptr));
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
            py::function leave_func(PyWeakref_GetObject(m_current_state->leave.get()), /* borrowed = */ true);
            if (leave_func.check()) {
                leave_func();
            }
            else {
                log_critical << "Invalid weakref of `leave` function of state: \"" << state << "\"";
            }
        }
        m_current_state = &next;
        py::function enter_func(PyWeakref_GetObject(m_current_state->enter.get()), /* borrowed = */ true);
        if (enter_func.check()) {
            enter_func();
        }
        else {
            log_critical << "Invalid weakref of `enter` function of state: \"" << state << "\"";
        }
    }

private: // fields
    /** All States of this Controller. */
    State::Map m_states;

    /** The current State of this Controller. */
    State* m_current_state;

public: // signals

    PySignal<> py_mouse_event;
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
