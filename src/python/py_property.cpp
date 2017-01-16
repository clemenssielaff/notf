#include "pybind11/pybind11.h"
namespace py = pybind11;

#include <functional>
#include <set>
#include <string>

#define NOTF_BINDINGS
#include "python/docstr.hpp"
#include "python/py_signal.hpp"
using namespace notf;

namespace std {

/** `less` specialization for py::weakref, used by std::set<py::weakref>. */
template <>
struct less<py::weakref> {
    bool operator()(const py::weakref& lhs, const py::weakref& rhs) const
    {
        return PyObject_RichCompareBool(lhs.ptr(), rhs.ptr(), Py_EQ) == 1;
    }
};

} // namespace  anonymous

class PyProperty : public receive_signals {
public:
    PyProperty(py::object value, std::string name)
        : m_name(std::move(name))
        , m_value(std::move(value))
        , m_is_dirty(false)
        , m_expression()
        , m_dependencies()
        , value_changed(py::cast(this), "value_changed")
    {
    }

    ~PyProperty() { _on_deletion(); }

    /** The name of this Property. */
    const std::string& get_name() const { return m_name; }

    /** The current value of this Property. */
    const py::object& get_value()
    {
        _make_clean();
        return m_value;
    }

    /** Tests whether this Property is currently defined by an Expression. */
    bool has_expression() const { return m_expression.ptr() != nullptr; }

    /** Sets this Property to a ground value.
     * If the Property is defined through an expression, manually setting the value will remove the expression.
     */
    void set_value(py::object value)
    {
        _drop_expression();
        _change_value(std::move(value));
    }

    /** Assign a new expression to this Property and execute it immediately. */
    void set_expression(py::function expression)
    {
        _drop_expression();
        m_expression = expression; // TODO: identify other Properties, connect their signals and store their dependencies
        _change_value(m_expression());
    }

    /** Adds a new dependency to this Property.
     * Every time a dependency Property is updated, this Property will try to re-evaluate its expression.
     * Always make sure that all Properties that this Property's expression depends on are registered as dependencies.
     * Existing dependencies are ignored.
     * @return  True if a dependency was added, false if it is already known.
     * @throw   std::runtime_error if the given Python object is not a PyProperty.
     */
    bool add_dependency(py::object dependency)
    {
        PyProperty* other = dependency.cast<PyProperty*>();
        if (!other) {
            throw std::runtime_error("`add_dependency` requires a Property as argument");
        }

        // make sure that each dependency is unique
        py::weakref weak_dep(dependency);
        if (m_dependencies.count(weak_dep) == 0) {
            m_dependencies.insert(weak_dep);
        } else {
            return false;
        }

        // connect all relevant signals
        connect_signal(other->_signal_test, [this]() { this->_signal_test(); });
        connect_signal(other->_signal_dirty, &PyProperty::_make_dirty);
        connect_signal(other->_signal_clean, &PyProperty::_make_clean);
        connect_signal(other->_on_deletion, &PyProperty::_drop_expression);
        return true;
    }

private: // methods
    /** Called when the user requests a change of this Property's value. */
    void _change_value(py::object value)
    {
        if (value != m_value) {
            m_value = value;
            _signal_test();
            _signal_dirty();
            _signal_clean();
            value_changed();
        }
    }

    /** Dirty propagation callback. */
    void _make_dirty()
    {
        if (!m_is_dirty && has_expression()) {
            m_is_dirty = true;
            _signal_dirty();
        }
    }

    /** Updates the value of this Property through its expression. */
    void _make_clean()
    {
        if (m_is_dirty) {
            assert(has_expression());
            const auto cache = m_value;
            m_value = m_expression();
            m_is_dirty = false;
            _signal_clean();
            if (cache != m_value) {
                value_changed();
            }
        }
    }

    /** Removes the current expression defining this Property without modifying its value. */
    void _drop_expression()
    {
        assert(!m_is_dirty);
        if (has_expression()) {
            m_expression = py::function();
            disconnect_all_connections();
            m_dependencies.clear();
        }
    }

private: // fields
    /** The name of this Property. */
    std::string m_name;

    /** The current value of this Property. */
    py::object m_value;

    /** Dirty flag, used to avoid redundant expression evaluations. */
    bool m_is_dirty;

    /** Expression defining this Property (can be empty). */
    py::function m_expression;

    /** All Properties that this one dependends on through its expression. */
    std::set<py::weakref> m_dependencies;

public: // signals
    /** Emitted when the value of this Property has changed. */
    PySignal<> value_changed;

private: // signals
    /** Emitted, when the Property is being deleted. */
    Signal<> _on_deletion;

    /** Test signal emitted first to check for cyclic dependencies without changing any state. */
    Signal<> _signal_test;

    /** Emitted, when the Property's value was changed by the user (directly or indirectly). */
    Signal<> _signal_dirty;

    /** Emitted, right after `_signal_dirty` signalling all dependent Properties to clean up. */
    Signal<> _signal_clean;
};

void produce_properties(pybind11::module& module)
{
    py::class_<PyProperty>(module, "Property")
        .def(py::init<py::object, std::string>())
        .def("get_name", &PyProperty::get_name, DOCSTR("The name of this Property."))
        .def("get_value", &PyProperty::get_value, DOCSTR("The current value of this Property."))
        .def("has_expression", &PyProperty::has_expression, DOCSTR("Tests whether this Property is currently defined by an Expression."))
        .def("set_value", &PyProperty::set_value, DOCSTR("Sets this Property to a ground value."), py::arg("value"))
        .def("set_expression", &PyProperty::set_expression, DOCSTR("Assign a new expression to this Property and execute it immediately."), py::arg("expression"))
        .def("add_dependency", &PyProperty::add_dependency, DOCSTR("Adds a new dependency to this Property."), py::arg("dependency"));
}
