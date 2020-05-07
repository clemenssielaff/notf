#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/vector2.hpp"
#include "docstr.hpp"
using namespace notf;

// squarebezierf ==================================================================================================== //

void produce_squarebezierf(pybind11::module& module)
{
    py::class_<SquareBezierf> Py_SquareBezierf(module, "SquareBezierf");

    // constructors
    Py_SquareBezierf.def(py::init<>());
    Py_SquareBezierf.def(py::init<float, float, float>(), py::arg("start"), py::arg("ctrl1"), py::arg("end"));

    // static constructors
    Py_SquareBezierf.def_static("line", &SquareBezierf::line, "Straight line with constant interpolation speed.", py::arg("start"), py::arg("end"));

    // inspection
    Py_SquareBezierf.def("get_weight", &SquareBezierf::get_weight, DOCSTR("Access to a weight of this Bezier, index must be in range [0, Order]."), py::arg("index"));
    Py_SquareBezierf.def("interpolate", &SquareBezierf::interpolate, DOCSTR("Bezier interpolation at position `t`, most useful in range [0, 1] but can be sampled outside as well."), py::arg("t"));

    // operators
    Py_SquareBezierf.def(py::self == py::self);
    Py_SquareBezierf.def(py::self != py::self);

    // builtins
    Py_SquareBezierf.def("__repr__", [](const SquareBezierf& bezier) {
        return fmt::format("pycnotf.SquareBezierf({}, {}, {})",
                           bezier.get_weight(0), bezier.get_weight(1), bezier.get_weight(2));
    });
}


// squarebezier2f ==================================================================================================== //

void produce_squarebezier2f(pybind11::module& module)
{
    py::class_<SquareBezier2f> Py_SquareBezier2f(module, "SquareBezier2f");

    // constructors
    Py_SquareBezier2f.def(py::init<>());
    Py_SquareBezier2f.def(py::init<V2f, V2f, V2f>(), py::arg("start"), py::arg("ctrl1"), py::arg("end"));

    // inspection
    Py_SquareBezier2f.def("get_dimension", &SquareBezier2f::get_dimension, DOCSTR("Access to a 1D Bezier that makes up this ParametricBezier."), py::arg("dimension"));
    Py_SquareBezier2f.def("get_vertex", &SquareBezier2f::get_vertex, DOCSTR("Access to a vertex of this Bezier, index must be in range [0, Order]."), py::arg("index"));
    Py_SquareBezier2f.def("interpolate", &SquareBezier2f::interpolate, DOCSTR("Bezier interpolation at position `t`, most useful in range [0, 1] but can be sampled outside as well."), py::arg("t"));

    // operators
    Py_SquareBezier2f.def(py::self == py::self);
    Py_SquareBezier2f.def(py::self != py::self);

    // builtins
    Py_SquareBezier2f.def("__repr__", [](const SquareBezier2f& bezier) {
        return fmt::format("pycnotf.SquareBezier2f(({}, {}), ({}, {}), ({}, {}))",
                           bezier.get_vertex(0).x(), bezier.get_vertex(0).y(),
                           bezier.get_vertex(1).x(), bezier.get_vertex(1).y(),
                           bezier.get_vertex(2).x(), bezier.get_vertex(2).y());
    });
}



// cubicbezierf ===================================================================================================== //

void produce_cubicbezierf(pybind11::module& module)
{
    py::class_<CubicBezierf> Py_CubicBezierf(module, "CubicBezierf");

    // constructors
    Py_CubicBezierf.def(py::init<>());
    Py_CubicBezierf.def(py::init<float, float, float, float>(), py::arg("start"), py::arg("ctrl1"), py::arg("ctrl2"), py::arg("end"));

    // static constructors
    Py_CubicBezierf.def_static("line", &CubicBezierf::line, "Straight line with constant interpolation speed.", py::arg("start"), py::arg("end"));

    // inspection
    Py_CubicBezierf.def("get_weight", &CubicBezierf::get_weight, DOCSTR("Access to a weight of this Bezier, index must be in range [0, Order]."), py::arg("index"));
    Py_CubicBezierf.def("get_derivate", &CubicBezierf::get_derivate, DOCSTR("The derivate Bezier, can be used to calculate the tangent."));
    Py_CubicBezierf.def("interpolate", &CubicBezierf::interpolate, DOCSTR("Bezier interpolation at position `t`, most useful in range [0, 1] but can be sampled outside as well."), py::arg("t"));

    // operators
    Py_CubicBezierf.def(py::self == py::self);
    Py_CubicBezierf.def(py::self != py::self);

    // builtins
    Py_CubicBezierf.def("__repr__", [](const CubicBezierf& bezier) {
        return fmt::format("pycnotf.CubicBezierf({}, {}, {}, {})",
                           bezier.get_weight(0), bezier.get_weight(1), bezier.get_weight(2), bezier.get_weight(3));
    });
}


// cubicbezier2f ==================================================================================================== //

void produce_cubicbezier2f(pybind11::module& module)
{
    py::class_<CubicBezier2f> Py_CubicBezier2f(module, "CubicBezier2f");

    // constructors
    Py_CubicBezier2f.def(py::init<>());
    Py_CubicBezier2f.def(py::init<V2f, V2f, V2f, V2f>(), py::arg("start"), py::arg("ctrl1"), py::arg("ctrl2"), py::arg("end"));

    // inspection
    Py_CubicBezier2f.def("get_dimension", &CubicBezier2f::get_dimension, DOCSTR("Access to a 1D Bezier that makes up this ParametricBezier."), py::arg("dimension"));
    Py_CubicBezier2f.def("get_vertex", &CubicBezier2f::get_vertex, DOCSTR("Access to a vertex of this Bezier, index must be in range [0, Order]."), py::arg("index"));
    Py_CubicBezier2f.def("get_derivate", &CubicBezier2f::get_derivate, DOCSTR("The derivate Bezier, can be used to calculate the tangent."));
    Py_CubicBezier2f.def("interpolate", &CubicBezier2f::interpolate, DOCSTR("Bezier interpolation at position `t`, most useful in range [0, 1] but can be sampled outside as well."), py::arg("t"));

    // operators
    Py_CubicBezier2f.def(py::self == py::self);
    Py_CubicBezier2f.def(py::self != py::self);

    // builtins
    Py_CubicBezier2f.def("__repr__", [](const CubicBezier2f& bezier) {
        return fmt::format("pycnotf.CubicBezier2f(({}, {}), ({}, {}), ({}, {}), ({}, {}))",
                           bezier.get_vertex(0).x(), bezier.get_vertex(0).y(),
                           bezier.get_vertex(1).x(), bezier.get_vertex(1).y(),
                           bezier.get_vertex(2).x(), bezier.get_vertex(2).y(),
                           bezier.get_vertex(3).x(), bezier.get_vertex(3).y());
    });
}

