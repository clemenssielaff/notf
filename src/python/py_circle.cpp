#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/circle.hpp"
#include "common/string.hpp"
#include "python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_circle(pybind11::module& module)
{
    py::class_<Circlef> PyCircle(module, "Circle");

    // constructors
    PyCircle.def(py::init<>());
    PyCircle.def(py::init<float>());
    PyCircle.def(py::init<const Vector2f&, float>());

    // static constructors
    PyCircle.def_static("null", &Circlef::zero, "The null Circle.");

    // properties
    PyCircle.def_readwrite("center", &Circlef::center);
    PyCircle.def_readwrite("radius", &Circlef::radius);
    PyCircle.def_property_readonly("diameter", &Circlef::diameter);
    PyCircle.def_property_readonly("circumfence", &Circlef::circumfence);
    PyCircle.def_property_readonly("area", &Circlef::area);

    // inspections
    PyCircle.def("is_null", &Circlef::is_zero, DOCSTR("Test, if this Circle is null; The null Circle has no area."));
    PyCircle.def("contains", &Circlef::contains, DOCSTR("Checks if this Circle contains a given point."), py::arg("point"));
    PyCircle.def("closest_point_to", &Circlef::closest_point_to, DOCSTR("Returns the closest point inside the Circle to a given target point."), py::arg("target"));

    // modification
    PyCircle.def("set_null", &Circlef::set_zero, DOCSTR("Sets this Circle to null."));

    // operators
    PyCircle.def(py::self == py::self);
    PyCircle.def(py::self != py::self);

    // representation
    PyCircle.def("__repr__", [](const Circlef& circle) {
        return string_format("notf.Circle([%f, %f], %f)", circle.center.x, circle.center.y, circle.radius);
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
