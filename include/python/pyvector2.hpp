#pragma once

namespace notf {
class Vector2;
}
using notf::Vector2;

#include "pybind11/pybind11.h"
namespace py = pybind11;

/** Adds bindings for the NoTF 'Vector2' class. */
py::class_<Vector2> produce_vector2(pybind11::module& module);
