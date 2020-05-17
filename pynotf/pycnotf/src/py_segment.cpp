#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/segment.hpp"
using namespace notf;

// clang-format off

// segment2f ======================================================================================================== //

void produce_segment2f(pybind11::module& module) {
    py::class_<Segment2f> Py_Segment2f(module, "Segment2f");

    // constructors
    Py_Segment2f.def(py::init<>());
    Py_Segment2f.def(py::init<V2f, V2f>(), py::arg("start"), py::arg("end"));

    // properties
    Py_Segment2f.def_readwrite("start", &Segment2f::start, DOCSTR("[V2f] Start point of the line Segment."));
    Py_Segment2f.def_readwrite("end", &Segment2f::end, DOCSTR("[V2f] End point of the line Segment."));

    // inspection
    Py_Segment2f.def("get_delta", &Segment2f::get_delta, DOCSTR("Difference vector between the end and start point of the Segment."));
    Py_Segment2f.def("get_length", &Segment2f::get_length, DOCSTR("The length of this line Segment."));
    Py_Segment2f.def("get_length_sq", &Segment2f::get_length_sq, DOCSTR("The squared length of this line Segment."));
    Py_Segment2f.def("get_bounding_rect", &Segment2f::get_bounding_rect, DOCSTR("The Aabr of this line Segment."));
    Py_Segment2f.def("is_zero", &Segment2f::is_zero, DOCSTR("Checks whether the Segment zero length."), py::arg("epsilon"));
    Py_Segment2f.def("is_parallel_to", &Segment2f::is_parallel_to, DOCSTR("Checks whether this Segment is parallel to another."), py::arg("other"));
    Py_Segment2f.def("is_orthogonal_to", &Segment2f::is_orthogonal_to, DOCSTR("Checks whether this Segment is orthogonal to another."), py::arg("other"));
    Py_Segment2f.def("contains", &Segment2f::contains, DOCSTR("Checks if this line Segment contains a given point."), py::arg("point"));
    Py_Segment2f.def("intersects", &Segment2f::intersects, DOCSTR("Quick tests if this Segment intersects another one."), py::arg("other"));
    Py_Segment2f.def("intersect", static_cast<std::optional<V2f>(Segment2f::*)(const Segment2f&) const>(&Segment2f::intersect), DOCSTR("The intersection of this line with another, iff they intersect at a unique point."), py::arg("other"));
    Py_Segment2f.def("get_closest_point", &Segment2f::get_closest_point, DOCSTR("The position on this line Segment that is closest to a given point."), py::arg("point"), py::arg("inside") = true);

    // builtins
    Py_Segment2f.def("__repr__", [](const Segment2f& segment) {
        return fmt::format("pycnotf.Segment2f({}, {} -> {}, {})",
                           segment.start.x(), segment.start.y(), segment.end.x(), segment.end.y());
    });
}

// clang-format on
