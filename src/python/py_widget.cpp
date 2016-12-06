#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/property_impl.hpp"
#include "core/state.hpp"

#define NOTF_BINDINGS
#include "core/widget.hpp"
using namespace notf;

void produce_widget(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<Widget, std::shared_ptr<Widget>> Py_Widget(module, "Widget", ancestor);

    Py_Widget.def(py::init<std::shared_ptr<StateMachine>>());

    Py_Widget.def("get_handle", &Widget::get_handle, "The Application-unique Handle of this Widget.");
}
