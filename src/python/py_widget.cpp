#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/widget.hpp"
using namespace notf;

void produce_widget(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<Widget, std::shared_ptr<Widget>> Py_Widget(module, "_Widget", ancestor);

    module.def("Widget", []() -> std::shared_ptr<Widget> {
        return Widget::create();
    }, "Creates a new Widget with an automatically assigned Handle.");

    module.def("Widget", [](const Handle handle) -> std::shared_ptr<Widget> {
        return Widget::create(handle);
    }, "Creates a new Widget with an explicitly assigned Handle.", py::arg("handle"));

    Py_Widget.def("get_handle", [](const std::shared_ptr<Widget>& widget) -> size_t {
        return widget->get_handle();
    }, "The Application-unique Handle of this Widget.");

    Py_Widget.def("add_component", &Widget::add_component, "Attaches a new Component to this Widget.", py::arg("component"));

    Py_Widget.def("set_claim", &Widget::set_claim, "Updates the Claim of this Widget.", py::arg("claim"));
}
