#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/property_impl.hpp"
#include "core/state.hpp"
#include "core/widget.hpp"
using namespace notf;

void produce_widget(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<Widget, std::shared_ptr<Widget>> Py_Widget(module, "_Widget", ancestor);

    module.def("Widget", [](std::shared_ptr<StateMachine> state_machine) -> std::shared_ptr<Widget> {
        return Widget::create(std::move(state_machine));
    }, "Creates a new Widget with an automatically assigned Handle.");

    module.def("Widget", [](std::shared_ptr<StateMachine> state_machine, const Handle handle) -> std::shared_ptr<Widget> {
        return Widget::create(std::move(state_machine), handle);
    }, "Creates a new Widget with an explicitly assigned Handle.", py::arg("state_machine"), py::arg("handle"));

    Py_Widget.def("get_handle", [](const std::shared_ptr<Widget>& widget) -> size_t {
        return widget->get_handle();
    }, "The Application-unique Handle of this Widget.");
}
