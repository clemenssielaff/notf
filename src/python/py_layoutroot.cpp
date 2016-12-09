#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/layout_root.hpp"
#include "python/docstr.hpp"
using namespace notf;

void produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<LayoutRoot, std::shared_ptr<LayoutRoot>> Py_LayoutRoot(module, "_LayoutRoot", ancestor);

    Py_LayoutRoot.def("set_item", &LayoutRoot::set_item, DOCSTR("Sets a new Item at the LayoutRoot."), py::arg("item"));
}
