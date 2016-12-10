#include <map>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "core/controller.hpp"
#include "python/docstr.hpp"
#include "python/pyobject_wrapper.hpp"
using namespace notf;

/* Trampoline Class ***************************************************************************************************/

class PyController : public AbstractController {

private: // struct
    struct State {
        using Map = std::map<std::string, State>;

        Map::const_iterator it;
        py::function enter;
        py::function leave;
    };

public: // methods
    PyController(std::shared_ptr<LayoutItem> root_item)
        : AbstractController(root_item)
        , m_states()
    {
    }

    virtual ~PyController() override;

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
        it->second = {it, enter, leave};
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
//            m_current_state->leave(py::cast(this));
        }
        m_current_state = &next;
        m_current_state->enter();//py::cast(this));
    }

private: // fields
    /** All States of this Controller. */
    State::Map m_states;

    /** The current State of this Controller. */
    State* m_current_state;
};
PyController::~PyController() {}

/* Custom Deallocator *************************************************************************************************/

static void (*py_controller_dealloc_orig)(PyObject*) = nullptr;
static void py_controller_dealloc(PyObject* object)
{
    auto instance = reinterpret_cast<py::detail::instance<PyController, std::shared_ptr<PyController>>*>(object);
    if (instance->holder.use_count() > 1) {
        instance->value->_set_pyobject(object);
        assert(object->ob_refcnt == 1);
        instance->holder.reset();

        // when we save the instance, we also need to incref the type so it doesn't die on us
        py_incref(&(object->ob_type->ob_base.ob_base));
    }
    else {
        assert(py_controller_dealloc_orig);
        py_controller_dealloc_orig(object);
    }
}

/* Bindings ***********************************************************************************************************/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_controller(pybind11::module& module, py::detail::generic_type Py_Item)
{
    py::class_<PyController, std::shared_ptr<PyController>> Py_Controller(module, "Controller", Py_Item);
    {
        PyTypeObject* py_controller_type = reinterpret_cast<PyTypeObject*>(Py_Controller.ptr());
        py_controller_dealloc_orig = py_controller_type->tp_dealloc;
        py_controller_type->tp_dealloc = py_controller_dealloc;
    }

    Py_Controller.def(py::init<std::shared_ptr<LayoutItem>>());

    Py_Controller.def("get_id", &PyController::get_id, DOCSTR("The application-unique ID of this Controller."));
    Py_Controller.def("has_parent", &PyController::has_parent, DOCSTR("Checks if this Item currently has a parent Item or not."));
    Py_Controller.def("get_opacity", &PyController::get_opacity, DOCSTR("Returns the opacity of this Item in the range [0 -> 1]."));
    Py_Controller.def("get_size", &PyController::get_size, DOCSTR("Returns the unscaled size of this Item in pixels."));
    //    Py_Widget.def("get_transform", &Widget::get_transform, "Returns this Item's transformation in the given space.", py::arg("space"));
    Py_Controller.def("get_claim", &PyController::get_claim, DOCSTR("The current Claim of this Item."));
    Py_Controller.def("is_visible", &PyController::is_visible, DOCSTR("Checks, if the Item is currently visible."));
    Py_Controller.def("get_current_state", &PyController::get_current_state, DOCSTR("Returns the name of the current State or an empty String, if the Controller doesn't have a State."));

    Py_Controller.def("add_state", &PyController::add_state, DOCSTR("Adds a new state to the Controller's state machine"), py::arg("name"), py::arg("enter_func"), py::arg("leave_func"));
    Py_Controller.def("transition_to", &PyController::transition_to, DOCSTR("Changes the current State and executes the relevant leave and enter-functions."), py::arg("state"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// TODO: finish PyController bindings
