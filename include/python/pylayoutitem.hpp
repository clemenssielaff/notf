#pragma once

namespace notf {
class LayoutItem;
}
using notf::LayoutItem;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'LayoutItem' class.
 * @param module    pybind11 module, modified by this factory.
 * @return          The created class object.
 */
py::class_<LayoutItem, std::shared_ptr<LayoutItem>> produce_layout_item(pybind11::module& module);
