#include <map>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/events/mouse_event.hpp"
#include "ext/python/docstr.hpp"
#include "ext/python/py_dict_utils.hpp"
#include "ext/python/py_fwd.hpp"
#include "ext/python/py_signal.hpp"
#include "ext/python/type_patches.hpp"
using namespace notf;

/* Trampoline Class ***************************************************************************************************/

class PyController : public AbstractController {

private: // struct
    struct State {
        using Map = std::map<std::string, State>;

        /** Iterator used to query the name of this State. */
        Map::const_iterator it;

        /** Weakref to the State's `enter` function. */
        std::unique_ptr<PyObject, decltype(&py_decref)> enter = {nullptr, py_decref};

        /** Weakref to the State's `leave` function. */
        std::unique_ptr<PyObject, decltype(&py_decref)> leave = {nullptr, py_decref};
    };

public: // methods
    PyController()
        : AbstractController()
        , m_states()
        , m_current_state(nullptr)
        , m_mouse_move(py::cast(this), "on_mouse_move")
        , m_mouse_button(py::cast(this), "on_mouse_button")
    {
        // connect Python signal translators
        connect_signal(on_mouse_move, [this](MouseEvent& event) {
            m_mouse_move.fire(event);
        });
        connect_signal(on_mouse_button, [this](MouseEvent& event) {
            m_mouse_button.fire(event);
        });
    }

    virtual ~PyController() override;

    using AbstractController::set_root_item;

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
            std::tie(it, success) = m_states.emplace(std::make_pair(name, State()));
            if (!success) {
                std::string msg = string_format("Cannot replace existing State \"%s\" in StateMachine", name.c_str());
                log_critical << msg;
                throw std::runtime_error(msg);
            }
            it->second.it = it;
        }

        { // store the two methods into a cache in the object's __dict__ so they don't get lost
            py::dict notf_cache = get_notf_cache(py::cast(this));
            py::dict cache = get_dict(notf_cache, s_state_cache_name.c_str());
            assert(cache.check());
            py::tuple handlers(PyTuple_Pack(2, enter.ptr(), leave.ptr()), /* borrowed = */ false);
            int success = PyDict_SetItemString(cache.ptr(), name.c_str(), handlers.ptr());
            assert(success == 0);
            UNUSED(success)
        }

        // ... and only keep weakrefs yourself, otherwise they will keep this object alive forever
        it->second.enter.reset(PyWeakref_NewRef(enter.ptr(), nullptr));
        it->second.leave.reset(PyWeakref_NewRef(leave.ptr(), nullptr));
    }
    // TODO: replace with c++11 py::weakref

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

    virtual void _set_pyobject(PyObject* object) override
    {
        Item::_set_pyobject(object);

        // restore states and signals
        _restore_states();
        py::object host(object, /* borrowed = */ true);
        m_mouse_move.restore(host);
        m_mouse_button.restore(host);
    }

private: // methods
    /** Restores the states after the Python object has been finalized an all weakrefs have been destroyed. */
    void _restore_states()
    {
        // get the state cache
        PyObject* self = _get_py_object();
        assert(self);
        py::dict notf_cache = get_notf_cache(py::object(self, /* borrowed = */ true));
        py::dict cache = get_dict(notf_cache, s_state_cache_name.c_str());
        assert(cache.check());

        // ... and use it to restore the targets
        for (auto& it : m_states) {
            py::tuple handlers(PyDict_GetItemString(cache.ptr(), it.first.c_str()), /* borrowed = */ true);
            assert(handlers.check());
            assert(handlers.size() == 2);
            it.second.enter.reset(PyWeakref_NewRef(py::object(handlers[0]).ptr(), nullptr));
            it.second.leave.reset(PyWeakref_NewRef(py::object(handlers[1]).ptr(), nullptr));
        }
    }

private: // fields
    /** All States of this Controller. */
    State::Map m_states;

    /** The current State of this Controller. */
    State* m_current_state;

private: // static fields
    /** Name of the cache field used for storing states. */
    static const std::string s_state_cache_name;

public: // signals
    /** Signal translator fired when the Controller receives an `on_mouse_move` event. */
    PySignal<MouseEvent&> m_mouse_move;

    /** Signal translator fired when the Controller receives an `on_mouse_button` event. */
    PySignal<MouseEvent&> m_mouse_button;
};

PyController::~PyController() {}

const std::string PyController::s_state_cache_name = "state_handlers";

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
    Py_Controller.def("set_root_item", &PyController::set_root_item, DOCSTR("Sets the LayoutItem at the root of the branch managedby this Controller."));

    Py_Controller.def("get_id", &PyController::get_id, DOCSTR("The application-unique ID of this Controller."));
    Py_Controller.def("has_parent", &PyController::has_parent, DOCSTR("Checks if this Item currently has a parent Item or not."));
    Py_Controller.def("get_current_state", &PyController::get_current_state, DOCSTR("Returns the name of the current State or an empty String, if the Controller doesn't have a State."));

    Py_Controller.def("add_state", &PyController::add_state, DOCSTR("Adds a new state to the Controller's state machine"), py::arg("name"), py::arg("enter_func"), py::arg("leave_func"));
    Py_Controller.def("transition_to", &PyController::transition_to, DOCSTR("Changes the current State and executes the relevant leave and enter-functions."), py::arg("state"));

    Py_Controller.def_readonly("on_mouse_move", &PyController::m_mouse_move);
    Py_Controller.def_readonly("on_mouse_button", &PyController::m_mouse_button);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
