#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "dynamic/layout/stack_layout.hpp"
using namespace notf;

void produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<StackLayout, std::shared_ptr<StackLayout>> Py_StackLayout(module, "_StackLayout", ancestor);

    module.def("StackLayout", [](const StackLayout::Direction direction) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction);
    }, "Creates a new StackLayout with an automatically assigned Handle.", py::arg("direction"));

    module.def("StackLayout", [](const StackLayout::Direction direction, const Handle handle) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction, handle);
    }, "Creates a new StackLayout with an explicitly assigned Handle.", py::arg("direction"), py::arg("handle"));

    Py_StackLayout.def("get_direction", &StackLayout::get_direction, "Direction in which items are stacked.");
    Py_StackLayout.def("get_alignment", &StackLayout::get_alignment, "Alignment of items in the main direction.");
    Py_StackLayout.def("get_cross_alignment", &StackLayout::get_cross_alignment, "Alignment of items in the cross direction.");
    Py_StackLayout.def("is_wrapping", &StackLayout::is_wrapping, "True if overflowing lines are wrapped.");
    Py_StackLayout.def("get_padding", &StackLayout::get_padding, "Padding around the Layout's border.");
    Py_StackLayout.def("get_spacing", &StackLayout::get_spacing, "Spacing between items.");

    Py_StackLayout.def("set_direction", &StackLayout::set_spacing, "Sets the spacing between items.", py::arg("spacing"));
    Py_StackLayout.def("set_alignment", &StackLayout::set_alignment, "Sets the alignment of items in the main direction.", py::arg("alignment"));
    Py_StackLayout.def("set_cross_alignment", &StackLayout::set_cross_alignment, "Sets the alignment of items in the cross direction.", py::arg("alignment"));
    Py_StackLayout.def("set_wrapping", &StackLayout::set_wrapping, "Defines if overflowing lines are wrapped.", py::arg("wrap"));
    Py_StackLayout.def("set_padding", &StackLayout::set_padding, "Sets the spacing between items.", py::arg("padding"));
    Py_StackLayout.def("set_spacing", &StackLayout::set_spacing, "Sets the spacing between items.", py::arg("spacing"));

    Py_StackLayout.def("add_item", &StackLayout::add_item, "Adds a new LayoutItem into the Layout.", py::arg("item"));
}
