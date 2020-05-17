#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/triangle.hpp"
using namespace notf;

// clang-format off

// trianglef ======================================================================================================== //

void produce_trianglef(pybind11::module& module) {
    py::class_<Trianglef> Py_Trianglef(module, "Trianglef");

    py::enum_<Orientation>(module, "Orientation")
        .value("UNDEFINED", Orientation::UNDEFINED)
        .value("CW", Orientation::CW)
        .value("CCW", Orientation::CCW);
}

// clang-format on
