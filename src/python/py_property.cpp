#include "pybind11/pybind11.h"
namespace py = pybind11;

#include <set>
#include <string>

#define NOTF_BINDINGS
#include "python/py_signal.hpp"
using namespace notf;

class PyProperty {
public:
    PyProperty(py::object value, std::string name)
        : m_name(std::move(name))
        , m_value(std::move(value))
        , m_expression()
        , m_dependencies()
        , value_changed(py::cast(this), "value_changed")
        , on_deletion(py::cast(this), "on_deletion")
    {
    }

    ~PyProperty() { on_deletion(); }

    /** Returns the current value of this Property. */
    const py::object& get_value() const { return m_value; }

    /** Tests whether this Property is currently defined by an Expression. */
    bool has_expression() const { return m_expression.ptr() != nullptr; }

    /** Updates the value of this Property.
     * If the Property is defined through an expression, manually setting the value will remove the expression.
     */
    void set_value(py::object value)
    {
        _drop_expression();
        m_value = std::move(value);
        value_changed();
    }

    /** Assigns a new expression to this Property and executes it immediately. */
    void set_expression(py::function expression)
    {
        m_expression = std::move(expression);
        // TODO: identify other Properties, connect their signals and store their dependencies
        _update_expression();
    }

private: // methods
    /** Updates the value of this Property through its expression. */
    void _update_expression()
    {
        if (has_expression()) {
            const py::object cache = m_value;
            m_value = m_expression();
            int result = PyObject_RichCompareBool(cache.ptr(), m_value.ptr(), Py_EQ);
            assert(result != -1);
            if (result == 0) {
                value_changed();
            }
        }
    }

    /** Removes the current expression defining this Property without modifying its value. */
    void _drop_expression()
    {
        m_expression = py::function();

        // disconnect from all dependencies
        for (const auto& dependency : m_dependencies) {
            const py::weakref& signal_weakref = dependency.first;
            py::object signal_obj(PyWeakref_GetObject(signal_weakref.ptr()), /* borrowed = */ true);
            try {
                PySignal<>* signal = signal_obj.cast<PySignal<>*>();
                signal->disconnect(dependency.second);
            }
            catch (py::cast_error) {
                log_critical << "Invalid weakref of dependent PyProperty's `value_changed` Signal in Property : \""
                             << m_name << "\"";
            }
        }
        m_dependencies.clear();
    }

private: // fields
    /** The name of this Property. */
    std::string m_name;

    /** Returns the current value of this Property. */
    py::object m_value;

    /** Expression defining this Property (can be empty). */
    py::function m_expression;

    /** PyProperties that are dependent on the value of this one. */
    std::set<std::pair<py::weakref, ConnectionID>> m_dependencies;

public: // signals
    /** Emitted when the value of this Property has changed. */
    PySignal<> value_changed;

    /** Emitted, when the Property is being deleted. */
    PySignal<> on_deletion;
};
