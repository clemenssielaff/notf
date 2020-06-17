#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/meta/function.hpp"
using namespace notf;

// clang-format off

// aabr ============================================================================================================= //

void produce_matrix3f(pybind11::module& module) {
    py::class_<M3f> Py_Matrix3f(module, "M3f");

    // constructors
    Py_Matrix3f.def(py::init<>());
    Py_Matrix3f.def(py::init([](float a, float b, float c, float d, float e, float f){ return M3f(V2f{a, b}, V2f{c, d}, V2f{e, f}); }), py::arg("a"), py::arg("b"), py::arg("c"), py::arg("d"), py::arg("e"), py::arg("f"));
    Py_Matrix3f.def(py::init<V2f, V2f, V2f>(), py::arg("first"), py::arg("second"), py::arg("third"));
    Py_Matrix3f.def(py::init<const M3f&>(), py::arg("other"));

    // static constructors
    Py_Matrix3f.def_static("all", &M3f::all, DOCSTR("A Matrix3 with both components set to the given value."), py::arg("value"));
    Py_Matrix3f.def_static("zero", &M3f::zero, DOCSTR("A Matrix3 with both components set to zero."));
    Py_Matrix3f.def_static("highest", &M3f::highest, DOCSTR("A Matrix3 with both components set to the highest possible float."));
    Py_Matrix3f.def_static("lowest", &M3f::lowest, DOCSTR("A Matrix3 with both components set to the lowest possible float."));
    Py_Matrix3f.def_static("diagonal", static_cast<M3f (*)(float, float)>(&M3f::diagonal), DOCSTR("A Matrix3 with given diagonal elements."), py::arg("a"), py::arg("d"));
    Py_Matrix3f.def_static("diagonal", static_cast<M3f (*)(float)>(&M3f::diagonal), DOCSTR("A Matrix3 with the given element in the diagonale."), py::arg("ad"));
    Py_Matrix3f.def_static("identity", &M3f::identity, DOCSTR("The identity matrix."));
    Py_Matrix3f.def_static("translation", static_cast<M3f (*)(V2f)>(&M3f::translation), DOCSTR("A translation matrix."), py::arg("translation"));
    Py_Matrix3f.def_static("translation", static_cast<M3f (*)(float, float)>(&M3f::translation), DOCSTR("A translation matrix."), py::arg("x"), py::arg("y"));
    Py_Matrix3f.def_static("rotation", &M3f::rotation, DOCSTR("Counterclockwise rotation in radian."), py::arg("angle"), py::arg("pivot") = V2f::zero());
    Py_Matrix3f.def_static("scale", static_cast<M3f (*)(float)>(&M3f::scale), DOCSTR("A uniform scale matrix."), py::arg("factor"));
    Py_Matrix3f.def_static("scale", static_cast<M3f (*)(float, float)>(&M3f::scale), DOCSTR("A non-uniform scale matrix."), py::arg("x"), py::arg("y"));
    Py_Matrix3f.def_static("scale", static_cast<M3f (*)(const V2f&)>(&M3f::scale), DOCSTR("A non-uniform scale matrix."), py::arg("factors"));
    Py_Matrix3f.def_static("squeeze", &M3f::squeeze, DOCSTR("A squeeze transformation."), py::arg("factor"));
    Py_Matrix3f.def_static("shear", static_cast<M3f (*)(float, float)>(&M3f::shear), DOCSTR("A non-uniform shear matrix."), py::arg("x"), py::arg("y"));
    Py_Matrix3f.def_static("shear", static_cast<M3f (*)(const V2f&)>(&M3f::shear), DOCSTR("A non-uniform shear matrix."), py::arg("factors"));
    Py_Matrix3f.def_static("reflection", static_cast<M3f (*)(const V2f&, V2f)>(&M3f::reflection), DOCSTR("Reflection over a line defined by two points."), py::arg("a"), py::arg("b"));
    Py_Matrix3f.def_static("reflection", static_cast<M3f (*)(V2f)>(&M3f::reflection), DOCSTR("Reflection over a line that passes through the origin in the given direction."), py::arg("direction"));
    Py_Matrix3f.def_static("reflection", static_cast<M3f (*)(float)>(&M3f::reflection), DOCSTR("Reflection over a line that passes through the origin at the given angle in radian."), py::arg("angle"));

    // inspection
    Py_Matrix3f.def("get_scale_factor", &M3f::get_scale_factor, DOCSTR("The scale factor."));
    Py_Matrix3f.def("get_determinant", &M3f::get_determinant, DOCSTR("The determinant."));
    Py_Matrix3f.def("get_inverse", &M3f::get_inverse, DOCSTR("The inverse of this matrix or the identity matrix on error."));
    Py_Matrix3f.def("is_zero", method_cast<M3f>(&M3f::is_zero), DOCSTR("Checks if this Matrix3 is all zero."), py::arg("epsilon") = precision_high<float>());
    Py_Matrix3f.def("is_approx", &M3f::is_approx, DOCSTR("Returns True, if other and self are approximately the same Matrix3."), py::arg("other"), py::arg("epsilon") = precision_high<float>());
    Py_Matrix3f.def("is_real", method_cast<M3f>(&M3f::is_real), DOCSTR("Checks, if this Matrix3 contains only real values."));
    Py_Matrix3f.def("contains_zero", method_cast<M3f>(&M3f::contains_zero), DOCSTR("Checks, if any component of this Matrix3 is a zero."), py::arg("epsilon") = precision_high<float>());
    Py_Matrix3f.def("get_max", method_cast<M3f>(&M3f::get_max), DOCSTR("Get the element-wise maximum of this and other."), py::arg("other"));
    Py_Matrix3f.def("get_min", method_cast<M3f>(&M3f::get_min), DOCSTR("Get the element-wise minimum of this and other."), py::arg("other"));
    Py_Matrix3f.def("get_sum", method_cast<M3f>(&M3f::get_sum), DOCSTR("Sum of all elements of this value."));
    Py_Matrix3f.def("get_abs", static_cast<M3f (M3f::*)() const &>(&M3f::get_abs), DOCSTR("Get a copy of this value with all elements set to their absolute value."));

    // modification
    Py_Matrix3f.def("translate", &M3f::translate, DOCSTR("Concatenate a translation to this transformation."), py::arg("delta"));
    Py_Matrix3f.def("rotate", &M3f::rotate, DOCSTR("Concatenate a counterclockwise rotation to this transformation."), py::arg("angle"));
    Py_Matrix3f.def("set_all", method_cast<M3f>(&M3f::set_all), DOCSTR("Sets all components of the Vector to the given value."), py::arg("value"));
    Py_Matrix3f.def("set_max", method_cast<M3f>(&M3f::set_max), DOCSTR("Set all elements of this value to the element-wise maximum of this and other."), py::arg("other"));
    Py_Matrix3f.def("set_min", method_cast<M3f const>(&M3f::set_min), DOCSTR("Set all elements of this value to the element-wise minimum of this and other."), py::arg("other"));
    Py_Matrix3f.def("set_abs", method_cast<M3f>(&M3f::set_abs), DOCSTR("Set all elements of this value to their absolute value."));

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

    // operators
    Py_Matrix3f.def(py::self * py::self);
    Py_Matrix3f.def(py::self *= py::self);
    Py_Matrix3f.def(py::self == py::self);
    Py_Matrix3f.def(py::self != py::self);
    Py_Matrix3f.def(py::self + py::self);
    Py_Matrix3f.def(py::self += py::self);
    Py_Matrix3f.def(py::self - py::self);
    Py_Matrix3f.def(py::self -= py::self);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    // builtins
    Py_Matrix3f.def("__len__", &M3f::get_size);
    Py_Matrix3f.def("__str__", [](const M3f& mat) { return fmt::format("({: #7.6g}, {: #7.6g}, {: #7.6g} / {: #7.6g}, {: #7.6g}, {: #7.6g} / {:8}, {:8}, {:8})", mat[0][0], mat[1][0], mat[2][0], mat[0][1], mat[1][1], mat[2][1], 0, 0, 1); });
    Py_Matrix3f.def("__repr__", [](const M3f& mat) { return fmt::format("pycnotf.M3f({: #7.6g}, {: #7.6g}, {: #7.6g} / {: #7.6g}, {: #7.6g}, {: #7.6g} / {:8}, {:8}, {:8})", mat[0][0], mat[1][0], mat[2][0], mat[0][1], mat[1][1], mat[2][1], 0, 0, 1); });
}

// clang-format on
