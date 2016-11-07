#pragma once

namespace notf {
class Component;
}
using notf::Component;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'Component' class.
 * @param module    pybind11 module, modified by this factory.
 * @return          The created class object.
 */
py::class_<Component, std::shared_ptr<Component>> produce_component(pybind11::module& module);
