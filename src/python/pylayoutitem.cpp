#include "python/pywidget.hpp"

#include "core/layout_item.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<LayoutItem, std::shared_ptr<LayoutItem>> produce_layout_item(pybind11::module& module)
{
    py::class_<LayoutItem, std::shared_ptr<LayoutItem>> Py_LayoutItem(module, "_LayoutItem");

    return Py_LayoutItem;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
