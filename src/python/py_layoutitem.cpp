#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/layout_item.hpp"
using namespace notf;

py::class_<LayoutItem, std::shared_ptr<LayoutItem>> produce_layout_item(pybind11::module& module)
{
    return py::class_<LayoutItem, std::shared_ptr<LayoutItem>>(module, "_LayoutItem");
}
