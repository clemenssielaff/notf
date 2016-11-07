#pragma once

namespace notf {
class Window;
}
using Window = notf::Window;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'Window' class.
 * @param module    pybind11 module, modified by this factory.
 * @return          The created class object.
 */
py::class_<Window, std::shared_ptr<Window>> produce_window(pybind11::module& module);
