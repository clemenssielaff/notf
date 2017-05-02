#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/controller.hpp"
#include "core/window_layout.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_layout_root(pybind11::module& module, py::detail::generic_type Py_LayoutItem)
{
    py::class_<WindowLayout, std::shared_ptr<WindowLayout>> Py_WindowLayout(module, "_WindowLayout", Py_LayoutItem);

    Py_WindowLayout.def("set_item", &WindowLayout::set_controller, DOCSTR("Sets a new Item at the WindowLayout."), py::arg("item"));
}
