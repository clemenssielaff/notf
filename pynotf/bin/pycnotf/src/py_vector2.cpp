#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/vector2.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif


// v2f ============================================================================================================== //

void produce_vector2(pybind11::module& module) {
    py::class_<V2f> Py_Vector2(module, "V2f");

    // constructors
    Py_Vector2.def(py::init<>());
    Py_Vector2.def(py::init<float, float>());
    Py_Vector2.def(py::init<V2f>());

    // static constructors
    Py_Vector2.def_static("all", &V2f::all, DOCSTR("A Vector2 with both components set to the given value"),
                          py::arg("value"));
    Py_Vector2.def_static("zero", &V2f::zero, DOCSTR("A Vector2 with both components set to zero"));
    Py_Vector2.def_static("x_axis", &V2f::x_axis, DOCSTR("Returns an unit Vector2 along the x-axis."));
    Py_Vector2.def_static("y_axis", &V2f::y_axis, DOCSTR("Returns an unit Vector2 along the y-axis."));

    // properties
    Py_Vector2.def_property("x", (const float& (V2f::*)() const) & V2f::x, (float& (V2f::*)()) & V2f::x);
    Py_Vector2.def_property("y", (const float& (V2f::*)() const) & V2f::y, (float& (V2f::*)()) & V2f::y);

    // swizzles
    Py_Vector2.def_property_readonly("xy", &V2f::xy);
    Py_Vector2.def_property_readonly("yx", &V2f::yx);

    // inspection
    Py_Vector2.def("is_zero", (bool (V2f::*)() const) & V2f::is_zero, DOCSTR("Checks if this Vector2 is the zero vector."));
    Py_Vector2.def("is_zero", (bool (V2f::*)(float) const) & V2f::is_zero, DOCSTR("Checks if this Vector2 is the zero vector."), py::arg("epsilon"));
    Py_Vector2.def("is_unit", &V2f::is_unit, DOCSTR("Checks whether this Vector2 is of unit magnitude."));
    Py_Vector2.def("is_parallel_to", &V2f::is_parallel_to, DOCSTR("Checks whether this Vector2 is parallel to other."), py::arg("other"));
    Py_Vector2.def("is_orthogonal_to", &V2f::is_orthogonal_to, DOCSTR("Checks whether this Vector2 is orthogonal to other."), py::arg("other"));
    Py_Vector2.def("get_angle_to", &V2f::get_angle_to, DOCSTR("Calculates the smallest angle between two Vector2s in radians."), py::arg("other"));
    Py_Vector2.def("get_direction_to", &V2f::get_direction_to, DOCSTR("Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between."), py::arg("other"));
    Py_Vector2.def("is_horizontal", &V2f::is_horizontal, DOCSTR("Tests if this Vector2 is parallel to the x-axis."));
    Py_Vector2.def("is_vertical", &V2f::is_vertical, DOCSTR("Tests if this Vector2 is parallel to the y-axis."));
    Py_Vector2.def("is_approx", (bool (V2f::*)(const V2f&) const) & V2f::is_approx, DOCSTR("Returns True, if other and self are approximately the same Vector2."), py::arg("other"));
    Py_Vector2.def("get_magnitude_sq", &V2f::get_magnitude_sq, DOCSTR("Returns the squared magnitude of this Vector2."));
    Py_Vector2.def("get_magnitude", &V2f::get_magnitude, DOCSTR("Returns the magnitude of this Vector2."));
    Py_Vector2.def("is_real", &V2f::is_real, DOCSTR("Checks, if this Vector2 contains only real values."));
    Py_Vector2.def("contains_zero", &V2f::contains_zero, DOCSTR("Checks, if any component of this Vector2 is a zero."));
    Py_Vector2.def("get_max", &V2f::get_max, DOCSTR("Get the element-wise maximum of this and other."), py::arg("other"));
    Py_Vector2.def("get_min", &V2f::get_min, DOCSTR("Get the element-wise minimum of this and other."), py::arg("other"));
    Py_Vector2.def("get_sum", &V2f::get_sum, DOCSTR("Sum of all elements of this value."));

    // modification
    Py_Vector2.def("set_all", &V2f::set_all, DOCSTR("Sets all components of the Vector to the given value."), py::arg("value"));
    Py_Vector2.def("dot", &V2f::dot, DOCSTR("Vector2 dot product."), py::arg("other"));
    Py_Vector2.def("cross", &V2f::cross, DOCSTR("Returns the cross product of this vector and another."), py::arg("other"));
    Py_Vector2.def("normalize", &V2f::normalize, DOCSTR("Normalizes this vector in-place."));
    Py_Vector2.def("fast_normalize", &V2f::fast_normalize, DOCSTR("Normalizes this vector in-place."));
    Py_Vector2.def("get_orthogonal", &V2f::get_orthogonal, DOCSTR("In-place rotation of this Vector2 90 degrees counter-clockwise."));
    Py_Vector2.def("get_rotated", (V2f(V2f::*)(float) const) & V2f::get_rotated, DOCSTR("Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians."), py::arg("angle"));
    Py_Vector2.def("get_rotated", (V2f(V2f::*)(float, const V2f&) const) & V2f::get_rotated, DOCSTR("Returns a copy this vector rotated around a pivot point by a given angle."), py::arg("angle"), py::arg("piot"));
    Py_Vector2.def("project_on", &V2f::project_on, DOCSTR("Creates a projection of this vector onto an infinite line whose direction is specified by other."), py::arg("other"));

    // operators
    Py_Vector2.def(py::self == py::self);
    Py_Vector2.def(py::self != py::self);
    Py_Vector2.def(py::self + py::self);
    Py_Vector2.def(py::self += py::self);
    Py_Vector2.def(py::self - py::self);
    Py_Vector2.def(py::self -= py::self);
    Py_Vector2.def(py::self * float());
    Py_Vector2.def(py::self *= float());
    Py_Vector2.def(py::self / float());
    Py_Vector2.def(py::self /= float());
    Py_Vector2.def(-py::self);

    // builtins
    Py_Vector2.def("__len__", &V2f::get_size);
    Py_Vector2.def("__repr__", [](const V2f& v2f) { return fmt::format("pycnotf.V2f({}, {})", v2f.x(), v2f.y()); });

    // free functions
    //    module.def("lerp", &lerp<V2f>, DOCSTR("Linear interpolation between two Vector2s."), py::arg("from"),
    //    py::arg("to"), py::arg("blend"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
