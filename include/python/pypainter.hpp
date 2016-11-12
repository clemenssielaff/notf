#pragma once

#include "pybind11/pybind11.h"
namespace py = pybind11;

namespace notf {
class Painter;
}
using notf::Painter;
class PyPainter;

/** Adds bindings for the NoTF 'Painter' class.
 * @param module    pybind11 module, modified by this factory.
 * @return          The created class object.
 */
py::class_<Painter, std::shared_ptr<PyPainter>> produce_painter(pybind11::module& module);
