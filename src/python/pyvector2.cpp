#include "python/pyvector2.hpp"

#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/string_utils.hpp"
#include "common/vector2.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<Vector2> produce_vector2(pybind11::module& module)
{
    py::class_<Vector2> Py_Vector2(module, "Vector2");

    // constructors
    Py_Vector2.def(py::init<>());
    Py_Vector2.def(py::init<Real, Real>());
    Py_Vector2.def(py::init<Vector2>());

    // static constructors
    Py_Vector2.def_static("fill", &Vector2::fill, "Returns a Vector2 with both components set to the given value", py::arg("value"));
    Py_Vector2.def_static("x_axis", &Vector2::x_axis, "Returns an unit Vector2 along the x-axis.");
    Py_Vector2.def_static("y_axis", &Vector2::y_axis, "Returns an unit Vector2 along the y-axis.");

    // properties
    Py_Vector2.def_readwrite("x", &Vector2::x);
    Py_Vector2.def_readwrite("y", &Vector2::y);

    // inspections
    Py_Vector2.def("is_zero", (bool (Vector2::*)() const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.");
    Py_Vector2.def("is_zero", (bool (Vector2::*)(Real) const) & Vector2::is_zero, "Checks if this Vector2 is the zero vector.", py::arg("epsilon"));
    Py_Vector2.def("is_unit", &Vector2::is_unit, "Checks whether this Vector2 is of unit magnitude.");
    Py_Vector2.def("is_parallel_to", &Vector2::is_parallel_to, "Checks whether this Vector2 is parallel to other.", py::arg("other"));
    Py_Vector2.def("is_orthogonal_to", &Vector2::is_orthogonal_to, "Checks whether this Vector2 is orthogonal to other.", py::arg("other"));
    Py_Vector2.def("angle", &Vector2::angle, "The angle in radians between the positive x-axis and the point given by this Vector2.");
    Py_Vector2.def("angle_to", &Vector2::angle_to, "Calculates the smallest angle between two Vector2s in radians.", py::arg("other"));
    Py_Vector2.def("direction_to", &Vector2::direction_to, "Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between.", py::arg("other"));
    Py_Vector2.def("is_horizontal", &Vector2::is_horizontal, "Tests if this Vector2 is parallel to the x-axis.");
    Py_Vector2.def("is_vertical", &Vector2::is_vertical, "Tests if this Vector2 is parallel to the y-axis.");
    Py_Vector2.def("is_approx", &Vector2::is_approx, "Returns True, if other and self are approximately the same Vector2.", py::arg("other"));
    Py_Vector2.def("is_not_approx", &Vector2::is_not_approx, "Returns True, if other and self are NOT approximately the same Vector2.", py::arg("other"));
    Py_Vector2.def("slope", &Vector2::slope, "Returns the slope of this Vector2.");
    Py_Vector2.def("magnitude_sq", &Vector2::magnitude_sq, "Returns the squared magnitude of this Vector2.");
    Py_Vector2.def("magnitude", &Vector2::magnitude, "Returns the magnitude of this Vector2.");
    Py_Vector2.def("is_real", &Vector2::is_real, "Checks, if this Vector2 contains only real values.");
    Py_Vector2.def("contains_zero", &Vector2::contains_zero, "Checks, if any component of this Vector2 is a zero.");

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
    Py_Vector2.def(py::self * Real());
    Py_Vector2.def(py::self *= Real());
    Py_Vector2.def(py::self / Real());
    Py_Vector2.def(py::self /= Real());
    Py_Vector2.def(-py::self);

    // modifiers
    Py_Vector2.def("set_zero", &Vector2::set_zero, "Sets all components of the Vector to zero.");
    Py_Vector2.def("inverted", &Vector2::inverted, "Returns an inverted copy of this Vector2.");
    Py_Vector2.def("invert", &Vector2::invert, "Inverts this Vector2 in-place.");
    Py_Vector2.def("dot", &Vector2::dot, "Vector2 dot product.", py::arg("other"));
    Py_Vector2.def("normalized", &Vector2::normalized, "Returns a normalized copy of this Vector2.");
    Py_Vector2.def("normalize", &Vector2::normalize, "Inverts this Vector2 in-place.");
    Py_Vector2.def("projected_on", &Vector2::projected_on, "Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"));
    Py_Vector2.def("project_on", &Vector2::project_on, "Projects this Vector2 onto an infinite line whose direction is specified by other.", py::arg("other"));
    Py_Vector2.def("orthogonal", &Vector2::orthogonal, "Creates an orthogonal 2D Vector to this one by rotating it 90 degree counter-clockwise.");
    Py_Vector2.def("orthogonalize", &Vector2::orthogonalize, "In-place rotation of this Vector2 90 degrees counter-clockwise.");
    Py_Vector2.def("rotated", &Vector2::rotated, "Returns a copy of this 2D Vector rotated counter-clockwise around its origin by a given angle in radians.", py::arg("angle"));
    Py_Vector2.def("rotate", &Vector2::rotate, "Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians.", py::arg("angle"));
    Py_Vector2.def("side_of", &Vector2::side_of, "The side of the other 2D Vector relative to direction of this 2D Vector (+1 = left, -1 = right).", py::arg("other"));

    // representation
    Py_Vector2.def("__repr__", [](const Vector2& vec) { return string_format("notf.Vector2(%f, %f)", vec.x, vec.y); });

    // free functions
    module.def("orthonormal_basis", (void (*)(Vector2&, Vector2&)) & orthonormal_basis, "Constructs a duo of mutually orthogonal, normalized vectors with 'a' as the reference vector", py::arg("a"), py::arg("b"));
    module.def("lerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & lerp, "Linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));
    module.def("nlerp", (Vector2(*)(const Vector2&, const Vector2&, Real)) & nlerp, "Normalized linear interpolation between two Vector2s.", py::arg("from"), py::arg("to"), py::arg("blend"));

    return Py_Vector2;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
