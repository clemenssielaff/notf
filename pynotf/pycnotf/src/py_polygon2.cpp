#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/polygon2.hpp"
using namespace notf;

// clang-format off

// polygon2f ======================================================================================================== //

void produce_polygon2f(pybind11::module& module) {
    py::class_<Polygon2f> Py_Polygon2f(module, "Polygon2f");

    // constructors
    Py_Polygon2f.def(py::init<>());
    Py_Polygon2f.def(py::init<std::vector<V2f>>(), py::arg("vertices"));

    // static
    Py_Polygon2f.def_static("contains_", static_cast<bool(*)(const std::vector<V2f>&, const V2f&) noexcept>(&Polygon2f::contains), DOCSTR("Tests if the given point is fully contained in this Polygon."), py::arg("vertices"), py::arg("point"));

    // inspection
    Py_Polygon2f.def("is_empty", &Polygon2f::is_empty, DOCSTR("Checks whether the Polygon has any vertices."));
    Py_Polygon2f.def("get_vertices", static_cast<const std::vector<V2f>& (Polygon2f::*)() const noexcept>(&Polygon2f::get_vertices), DOCSTR("The vertices of this Polygon."));
    Py_Polygon2f.def("get_center", static_cast<V2f(Polygon2f::*)() const noexcept>(&Polygon2f::get_center), DOCSTR("The center point of the Polygon."));
    Py_Polygon2f.def("get_aabr", static_cast<Aabrf(Polygon2f::*)() const noexcept>(&Polygon2f::get_aabr), DOCSTR("The axis-aligned bounding rect of the Polygon."));
    Py_Polygon2f.def("is_convex", static_cast<bool(Polygon2f::*)() const noexcept>(&Polygon2f::is_convex), DOCSTR("Checks if this Polygon is convex."));
    Py_Polygon2f.def("is_concave", static_cast<bool(Polygon2f::*)() const noexcept>(&Polygon2f::is_concave), DOCSTR("Checks if this Polygon is concave."));
    Py_Polygon2f.def("get_orientation", static_cast<Orientation(Polygon2f::*)() const noexcept>(&Polygon2f::get_orientation), DOCSTR("Calculates the orientation of a simple Polygon."));
    Py_Polygon2f.def("contains", static_cast<bool(Polygon2f::*)(const V2f&) const noexcept>(&Polygon2f::contains), DOCSTR("Tests if the given point is fully contained in this Polygon."), py::arg("point"));
    Py_Polygon2f.def("is_approx", &Polygon2f::is_approx, DOCSTR("Tests whether this Polygon is vertex-wise approximate to another."), py::arg("other"), py::arg("epsilon") = precision_high<float>());
    Py_Polygon2f.def("optimize", static_cast<void(Polygon2f::*)()>(&Polygon2f::optimize), DOCSTR("Remove all vertices that do not add additional corners to the Polygon."));

    // operators
    Py_Polygon2f.def(py::self == py::self);
    Py_Polygon2f.def(py::self != py::self);

//    // builtins
//    Py_Segment2f.def("__repr__", [](const Segment2f& segment) {
//        return fmt::format("pycnotf.Segment2f({}, {} -> {}, {})",
//                           segment.start.x(), segment.start.y(), segment.end.x(), segment.end.y());
//    });
}

// clang-format on
