#include "python/pywidget.hpp"

#include "dynamic/layout/stack_layout.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<StackLayout, std::shared_ptr<StackLayout>> produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<StackLayout, std::shared_ptr<StackLayout>> Py_StackLayout(module, "_StackLayout", ancestor);

    module.def("StackLayout", [](const STACK_DIRECTION direction) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction);
    }, "Creates a new StackLayout with an automatically assigned Handle.", py::arg("direction"));

    module.def("StackLayout", [](const STACK_DIRECTION direction, const Handle handle) -> std::shared_ptr<StackLayout> {
        return StackLayout::create(direction, handle);
    }, "Creates a new StackLayout with an explicitly assigned Handle.", py::arg("direction"), py::arg("handle"));

    Py_StackLayout.def("add_item", &StackLayout::add_item, "Adds a new LayoutItem into the Layout.", py::arg("item"));

    return Py_StackLayout;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
