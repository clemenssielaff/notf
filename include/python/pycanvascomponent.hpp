#pragma once

namespace notf {
class CanvasComponent;
}
using notf::CanvasComponent;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'CanvasComponent' class.
 * @param module    pybind11 module, modified by this factory.
 * @param ancestor  Binding class generated for the ancestor of this class.
 * @return          The created class object.
 */
py::class_<CanvasComponent, std::shared_ptr<CanvasComponent>> produce_canvas_component(pybind11::module& module, py::detail::generic_type ancestor);
