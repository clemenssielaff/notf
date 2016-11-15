#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "dynamic/layout/stack_layout.hpp"
using namespace notf;

void produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<StackLayout, std::shared_ptr<StackLayout>> Py_StackLayout(module, "_StackLayout", ancestor);

    module.def("StackLayout", [](const STACK_DIRECTION direction) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction);
    }, "Creates a new StackLayout with an automatically assigned Handle.", py::arg("direction"));

    module.def("StackLayout", [](const STACK_DIRECTION direction, const Handle handle) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction, handle);
    }, "Creates a new StackLayout with an explicitly assigned Handle.", py::arg("direction"), py::arg("handle"));

    Py_StackLayout.def("add_item", &StackLayout::add_item, "Adds a new LayoutItem into the Layout.", py::arg("item"));
}
