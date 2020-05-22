#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/vector2.hpp"
#include "notf/meta/function.hpp"
using namespace notf;

// clang-format off

// v2f ============================================================================================================== //

void produce_vector2(pybind11::module& module) {
    py::class_<V2f> Py_Vector2(module, "V2f");

    // constructors
    Py_Vector2.def(py::init<>());
    Py_Vector2.def(py::init<float, float>(), py::arg("x"), py::arg("y"));
    Py_Vector2.def(py::init<const V2f&>(), py::arg("other"));

    // static constructors
    Py_Vector2.def_static("all", &V2f::all, DOCSTR("A Vector2 with both components set to the given value."), py::arg("value"));
    Py_Vector2.def_static("zero", &V2f::zero, DOCSTR("A Vector2 with both components set to zero."));
    Py_Vector2.def_static("highest", &V2f::highest, DOCSTR("A Vector2 with both components set to the highest possible float."));
    Py_Vector2.def_static("lowest", &V2f::lowest, DOCSTR("A Vector2 with both components set to the lowest possible float."));
    Py_Vector2.def_static("x_axis", &V2f::x_axis, DOCSTR("Returns an unit Vector2 along the x-axis."));
    Py_Vector2.def_static("y_axis", &V2f::y_axis, DOCSTR("Returns an unit Vector2 along the y-axis."));

    // properties
    Py_Vector2.def_property("x", static_cast<const float& (V2f::*)() const>(&V2f::x), [](V2f& v, float value){v.x() = value;}, DOCSTR("[float] The first element in the vector."));
    Py_Vector2.def_property("y", static_cast<const float& (V2f::*)() const>(&V2f::y), [](V2f& v, float value){v.y() = value;}, DOCSTR("[float] The second element in the vector."));

    // swizzles
    Py_Vector2.def_property_readonly("xy", &V2f::xy, DOCSTR("[V2f] XY Swizzle."));
    Py_Vector2.def_property_readonly("yx", &V2f::yx, DOCSTR("[V2f] YX Swizzle."));
    Py_Vector2.def_property_readonly("xx", &V2f::xx, DOCSTR("[V2f] XX Swizzle."));
    Py_Vector2.def_property_readonly("yy", &V2f::yy, DOCSTR("[V2f] YY Swizzle."));

    // inspection
    Py_Vector2.def("is_zero", method_cast<V2f>(&V2f::is_zero), DOCSTR("Checks if this Vector2 is the zero vector."), py::arg("epsilon") = precision_high<float>());
    Py_Vector2.def("is_unit", method_cast<V2f>(&V2f::is_unit), DOCSTR("Checks whether this Vector2 is of unit magnitude."));
    Py_Vector2.def("is_parallel_to", &V2f::is_parallel_to, DOCSTR("Checks whether this Vector2 is parallel to other."), py::arg("other"));
    Py_Vector2.def("is_orthogonal_to", &V2f::is_orthogonal_to, DOCSTR("Checks whether this Vector2 is orthogonal to other."), py::arg("other"));
    Py_Vector2.def("get_angle_to", &V2f::get_angle_to, DOCSTR("Calculates the smallest angle between two Vector2s in radians."), py::arg("other"));
    Py_Vector2.def("get_direction_to", &V2f::get_direction_to, DOCSTR("Tests if the other Vector2 is collinear (1) to this, opposite (-1) or something in between."), py::arg("other"));
    Py_Vector2.def("is_horizontal", &V2f::is_horizontal, DOCSTR("Tests if this Vector2 is parallel to the x-axis."));
    Py_Vector2.def("is_vertical", &V2f::is_vertical, DOCSTR("Tests if this Vector2 is parallel to the y-axis."));
    Py_Vector2.def("is_approx", &V2f::is_approx, DOCSTR("Returns True, if other and self are approximately the same Vector2."), py::arg("other"), py::arg("epsilon") = precision_high<float>());
    Py_Vector2.def("get_magnitude_sq", method_cast<V2f>(&V2f::get_magnitude_sq), DOCSTR("Returns the squared magnitude of this Vector2."));
    Py_Vector2.def("get_magnitude", method_cast<V2f>(&V2f::get_magnitude), DOCSTR("Returns the magnitude of this Vector2."));
    Py_Vector2.def("is_real", method_cast<V2f>(&V2f::is_real), DOCSTR("Checks, if this Vector2 contains only real values."));
    Py_Vector2.def("contains_zero", method_cast<V2f>(&V2f::contains_zero), DOCSTR("Checks, if any component of this Vector2 is a zero."), py::arg("epsilon") = precision_high<float>());
    Py_Vector2.def("get_max", method_cast<V2f>(&V2f::get_max), DOCSTR("Get the element-wise maximum of this and other."), py::arg("other"));
    Py_Vector2.def("get_min", method_cast<V2f>(&V2f::get_min), DOCSTR("Get the element-wise minimum of this and other."), py::arg("other"));
    Py_Vector2.def("get_sum", method_cast<V2f>(&V2f::get_sum), DOCSTR("Sum of all elements of this value."));
    Py_Vector2.def("get_abs", static_cast<V2f (V2f::*)() const &>(&V2f::get_abs), DOCSTR("Get a copy of this value with all elements set to their absolute value."));

    // modification
    Py_Vector2.def("set_all", method_cast<V2f>(&V2f::set_all), DOCSTR("Sets all components of the Vector to the given value."), py::arg("value"));
    Py_Vector2.def("set_max", method_cast<V2f>(&V2f::set_max), DOCSTR("Set all elements of this value to the element-wise maximum of this and other."), py::arg("other"));
    Py_Vector2.def("set_min", method_cast<V2f const>(&V2f::set_min), DOCSTR("Set all elements of this value to the element-wise minimum of this and other."), py::arg("other"));
    Py_Vector2.def("set_abs", method_cast<V2f>(&V2f::set_abs), DOCSTR("Set all elements of this value to their absolute value."));
    Py_Vector2.def("dot", method_cast<V2f>(&V2f::dot), DOCSTR("Vector2 dot product."), py::arg("other"));
    Py_Vector2.def("cross", &V2f::cross, DOCSTR("Returns the cross product of this vector and another."), py::arg("other"));
    Py_Vector2.def("normalize", method_cast<V2f>(&V2f::normalize), DOCSTR("Normalizes this vector in-place."));
    Py_Vector2.def("fast_normalize", method_cast<V2f>(&V2f::fast_normalize), DOCSTR("Normalizes this vector in-place."));
    Py_Vector2.def("get_orthogonal", &V2f::get_orthogonal, DOCSTR("In-place rotation of this Vector2 90 degrees counter-clockwise."));
    Py_Vector2.def("get_rotated", static_cast<V2f (V2f::*)(const float) const>(&V2f::get_rotated), DOCSTR("Rotates this Vector2 counter-clockwise in-place around its origin by a given angle in radians."), py::arg("angle"));
    Py_Vector2.def("get_rotated", static_cast<V2f (V2f::*)(float, const V2f&) const>(&V2f::get_rotated), DOCSTR("Returns a copy this vector rotated around a pivot point by a given angle."), py::arg("angle"), py::arg("piot"));
    Py_Vector2.def("project_on", &V2f::project_on, DOCSTR("Creates a projection of this vector onto an infinite line whose direction is specified by other."), py::arg("other"));

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

    // operators
    Py_Vector2.def(py::self == py::self);
    Py_Vector2.def(py::self != py::self);
    Py_Vector2.def(py::self + py::self);
    Py_Vector2.def(py::self += py::self);
    Py_Vector2.def(py::self - py::self);
    Py_Vector2.def(py::self -= py::self);
    Py_Vector2.def(py::self * float());
    Py_Vector2.def(float() * py::self);
    Py_Vector2.def(py::self *= float());
    Py_Vector2.def(py::self / float());
    Py_Vector2.def(py::self /= float());
    Py_Vector2.def(-py::self);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    // builtins
    Py_Vector2.def("__len__", &V2f::get_size);
    Py_Vector2.def("__str__", [](const V2f& v2f) { return fmt::format("({}, {})", v2f.x(), v2f.y()); });
    Py_Vector2.def("__repr__", [](const V2f& v2f) { return fmt::format("pycnotf.V2f({}, {})", v2f.x(), v2f.y()); });

    // functions
    module.def("shoelace", static_cast<float(*)(const V2f&, const V2f&, const V2f&)>(&shoelace), "The Shoelace formula.", py::arg("a"), py::arg("b"), py::arg("c"));
}

// clang-format on
