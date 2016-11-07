#pragma once

namespace notf {
class StackLayout;
}
using notf::StackLayout;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'StackLayout' class.
 * @param module    pybind11 module, modified by this factory.
 * @param ancestor  Binding class generated for the ancestor of this class.
 * @return          The created class object.
 */
py::class_<StackLayout, std::shared_ptr<StackLayout>> produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor);
