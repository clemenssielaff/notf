#pragma once

namespace notf {
class LayoutRoot;
}
using notf::LayoutRoot;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'LayoutRoot' class.
 * @param module    pybind11 module, modified by this factory.
 * @param ancestor  Binding class generated for the ancestor of this class.
 * @return          The created class object.
 */
py::class_<LayoutRoot, std::shared_ptr<LayoutRoot>> produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor);
