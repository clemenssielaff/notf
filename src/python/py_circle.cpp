#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/circle.hpp"
#include "common/string_utils.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_circle(pybind11::module& module)
{
    py::class_<Circle> PyCircle(module, "Circle");

    // constructors
    PyCircle.def(py::init<>());
    PyCircle.def(py::init<float>());
    PyCircle.def(py::init<const Vector2&, float>());

    // static constructors
    PyCircle.def_static("null", &Circle::null, "The null Circle.");

    // properties
    PyCircle.def_readwrite("center", &Circle::center);
    PyCircle.def_readwrite("radius", &Circle::radius);
    PyCircle.def_property_readonly("diameter", &Circle::diameter);
    PyCircle.def_property_readonly("circumfence", &Circle::circumfence);
    PyCircle.def_property_readonly("area", &Circle::area);

    // inspections
    PyCircle.def("is_null", &Circle::is_null, "Test, if this Circle is null; The null Circle has no area.");
    PyCircle.def("contains", &Circle::contains, "Checks if this Circle contains a given point.", py::arg("point"));
    PyCircle.def("closest_point_to", &Circle::closest_point_to, "Returns the closest point inside the Circle to a given target point.", py::arg("target"));

    // modification
    PyCircle.def("set_null", &Circle::set_null, "Sets this Circle to null.");

    // operators
    PyCircle.def(py::self == py::self);
    PyCircle.def(py::self != py::self);

    // representation
    PyCircle.def("__repr__", [](const Circle& circle) {
        return string_format("notf.Circle([%f, %f], %f)", circle.center.x, circle.center.y, circle.radius);
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
