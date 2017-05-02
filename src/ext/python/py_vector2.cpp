#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/string.hpp"
#include "common/vector2.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_vector2(pybind11::module& module)
{
    py::class_<Vector2f> Py_Vector2(module, "Vector2");

    // constructors
    Py_Vector2.def(py::init<>());
    Py_Vector2.def(py::init<float, float>());
    Py_Vector2.def(py::init<Vector2f>());

    // static constructors
    Py_Vector2.def_static("fill", &Vector2f::fill, DOCSTR("Returns a Vector2 with both components set to the given value"), py::arg("value"));
    Py_Vector2.def_static("x_axis", &Vector2f::x_axis, DOCSTR("Returns an unit Vector2 along the x-axis."));
    Py_Vector2.def_static("y_axis", &Vector2f::y_axis, DOCSTR("Returns an unit Vector2 along the y-axis."));

    // properties
    Py_Vector2.def_readwrite("x", &Vector2f::x);
    Py_Vector2.def_readwrite("y", &Vector2f::y);

    // inspections
    Py_Vector2.def("is_zero", (bool (Vector2f::*)() const) & Vector2f::is_zero, DOCSTR("Checks if this Vector2 is the zero vector."));
    Py_Vector2.def("is_zero", (bool (Vector2f::*)(float) const) & Vector2f::is_zero, DOCSTR("Checks if this Vector2 is the zero vector."), py::arg("epsilon"));
    Py_Vector2.def("is_unit", &Vector2f::is_unit, DOCSTR("Checks whether this Vector2 is of unit magnitude."));
    Py_Vector2.def("is_parallel_to", &Vector2f::is_parallel_to, DOCSTR("Checks whether this Vector2 is parallel to other."), py::arg("other"));
    Py_Vector2.def("is_orthogonal_to", &Vector2f::is_orthogonal_to, DOCSTR("Checks whether this Vector2 is orthogonal to other."), py::arg("other"));
    Py_Vector2.def("angle", &Vector2f::angle, DOCSTR("The angle in radians between the positive x-axis and the point given by this Vector2."));
    Py_Vector2.def("angle_to", &Vector2f::angle_to, DOCSTR("Calculates the smallest angle between two Vector2s in radians."), py::arg("other"));
    Py_Vector2.def("direction_to", &Vector2f::direction_to, DOCSTR("Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between."), py::arg("other"));
    Py_Vector2.def("is_horizontal", &Vector2f::is_horizontal, DOCSTR("Tests if this Vector2 is parallel to the x-axis."));
    Py_Vector2.def("is_vertical", &Vector2f::is_vertical, DOCSTR("Tests if this Vector2 is parallel to the y-axis."));
    Py_Vector2.def("is_approx", (bool (Vector2f::*)(const Vector2f&) const) & Vector2f::is_approx, DOCSTR("Returns True, if other and self are approximately the same Vector2."), py::arg("other"));
    Py_Vector2.def("slope", &Vector2f::slope, DOCSTR("Returns the slope of this Vector2."));
    Py_Vector2.def("magnitude_sq", &Vector2f::magnitude_sq, DOCSTR("Returns the squared magnitude of this Vector2."));
    Py_Vector2.def("magnitude", &Vector2f::magnitude, DOCSTR("Returns the magnitude of this Vector2."));
    Py_Vector2.def("is_real", &Vector2f::is_real, DOCSTR("Checks, if this Vector2 contains only real values."));
    Py_Vector2.def("contains_zero", &Vector2f::contains_zero, DOCSTR("Checks, if any component of this Vector2 is a zero."));

    // operators
    Py_Vector2.def(py::self == py::self);
    Py_Vector2.def(py::self != py::self);
    Py_Vector2.def(py::self + py::self);
    Py_Vector2.def(py::self += py::self);
    Py_Vector2.def(py::self - py::self);
    Py_Vector2.def(py::self -= py::self);
    Py_Vector2.def(py::self * py::self);
    Py_Vector2.def(py::self *= py::self);
    Py_Vector2.def(py::self / py::self);
    Py_Vector2.def(py::self /= py::self);
    Py_Vector2.def(py::self * float());
    Py_Vector2.def(py::self *= float());
    Py_Vector2.def(py::self / float());
    Py_Vector2.def(py::self /= float());
    Py_Vector2.def(-py::self);

    // modifiers
    Py_Vector2.def("set_null", &Vector2f::set_zero, DOCSTR("Sets all components of the Vector to zero."));
    Py_Vector2.def("inverted", &Vector2f::inverse, DOCSTR("Returns an inverted copy of this Vector2."));
    Py_Vector2.def("invert", &Vector2f::invert, DOCSTR("Inverts this Vector2 in-place."));
    Py_Vector2.def("dot", &Vector2f::dot, DOCSTR("Vector2 dot product."), py::arg("other"));
    Py_Vector2.def("normalized", &Vector2f::normalized, DOCSTR("Returns a normalized copy of this Vector2."));
    Py_Vector2.def("normalize", &Vector2f::normalize, DOCSTR("Inverts this Vector2 in-place."));
    Py_Vector2.def("projected_on", &Vector2f::projected_on, DOCSTR("Creates a projection of this Vector2 onto an infinite line whose direction is specified by other."), py::arg("other"));
    Py_Vector2.def("project_on", &Vector2f::project_on, DOCSTR("Projects this Vector2 onto an infinite line whose direction is specified by other."), py::arg("other"));
    Py_Vector2.def("orthogonal", &Vector2f::get_orthogonal, DOCSTR("Creates an orthogonal 2D Vector to this one by rotating it 90 degree counter-clockwise."));
    Py_Vector2.def("orthogonalize", &Vector2f::orthogonalize, DOCSTR("In-place rotation of this Vector2 90 degrees counter-clockwise."));
    Py_Vector2.def("rotated", &Vector2f::get_rotated, DOCSTR("Returns a copy of this 2D Vector rotated counter-clockwise around its origin by a given angle in radians."), py::arg("angle"));
    Py_Vector2.def("rotate", &Vector2f::rotate, DOCSTR("Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians."), py::arg("angle"));
    Py_Vector2.def("side_of", &Vector2f::get_side_of, DOCSTR("The side of the other 2D Vector relative to direction of this 2D Vector (+1 = left, -1 = right)."), py::arg("other"));

    // representation
    Py_Vector2.def("__repr__", [](const Vector2f& vec) { return string_format("notf.Vector2(%f, %f)", vec.x, vec.y); });

    // free functions
    module.def("lerp", (Vector2f(*)(const Vector2f&, const Vector2f&, float)) & lerp, DOCSTR("Linear interpolation between two Vector2s."), py::arg("from"), py::arg("to"), py::arg("blend"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
