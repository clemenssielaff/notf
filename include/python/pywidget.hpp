#pragma once

namespace notf {
class Widget;
}
using notf::Widget;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'Widget' class.
 * @param module    pybind11 module, modified by this factory.
 * @param ancestor  Binding class generated for the ancestor of this class.
 * @return          The created class object.
 */
py::class_<Widget, std::shared_ptr<Widget>> produce_widget(pybind11::module& module, py::detail::generic_type ancestor);
