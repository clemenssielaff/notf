#include "python/pywidget.hpp"

#include "core/layout_root.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<LayoutRoot, std::shared_ptr<LayoutRoot>> produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<LayoutRoot, std::shared_ptr<LayoutRoot>> Py_LayoutRoot(module, "_LayoutRoot", ancestor);

    Py_LayoutRoot.def("set_item", &LayoutRoot::set_item, "Sets a new Item at the LayoutRoot.", py::arg("item"));

    return Py_LayoutRoot;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif









