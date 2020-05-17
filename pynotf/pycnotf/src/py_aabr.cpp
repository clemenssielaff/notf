#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/geo/aabr.hpp"
#include "notf/meta/function.hpp"
using namespace notf;

// clang-format off

// aabr ============================================================================================================= //

void produce_aabr(pybind11::module& module) {
    py::class_<Aabrf> Py_Aabrf(module, "Aabrf");

    // constructors
    Py_Aabrf.def(py::init<>());
    Py_Aabrf.def(py::init<float, float, float, float>(), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"));
    Py_Aabrf.def(py::init<const V2f&, float, float>(), py::arg("bottom_left"), py::arg("width"), py::arg("height"));
    Py_Aabrf.def(py::init<const V2f&, const Size2f&>(), py::arg("bottom_left"), py::arg("size"));
    Py_Aabrf.def(py::init<const Size2f&>(), py::arg("size"));
    Py_Aabrf.def(py::init<const V2f&, const V2f&>(), py::arg("a"), py::arg("b"));
    Py_Aabrf.def(py::init<const Aabrf&>(), py::arg("other"));

    // static constructors
    Py_Aabrf.def_static("zero", &Aabrf::zero, "The null Aabr.");
    Py_Aabrf.def_static("largest", &Aabrf::largest, "The largest representable Aabr.");
    Py_Aabrf.def_static("wrongest", &Aabrf::wrongest, "The 'most wrong' Aabr (maximal negative area).");
    Py_Aabrf.def_static("centered", &Aabrf::centered, "Returns an Aabr of a given size, with zero in the center.", py::arg("size"));

    // properties
    Py_Aabrf.def_property("left", &Aabrf::get_left, &Aabrf::set_left, DOCSTR("[float] The x-coordinate of the left edge of this Aabr."));
    Py_Aabrf.def_property("right", &Aabrf::get_right, &Aabrf::set_right, DOCSTR("[float] The x-coordinate of the right edge of this Aabr."));
    Py_Aabrf.def_property("top", &Aabrf::get_top, &Aabrf::set_top, DOCSTR("[float] The y-coordinate of the top edge of this Aabr."));
    Py_Aabrf.def_property("bottom", &Aabrf::get_bottom, &Aabrf::set_bottom, DOCSTR("[float] The y-coordinate of the bottomedge of this Aabr."));
    Py_Aabrf.def_property("center", &Aabrf::get_center, &Aabrf::set_center, DOCSTR("[V2f] The center of this Aabr."));
    Py_Aabrf.def_property("x", &Aabrf::get_center_x, &Aabrf::set_center_x, DOCSTR("[float] The x-coordinate of the center of this Aabr."));
    Py_Aabrf.def_property("y", &Aabrf::get_center_y, &Aabrf::set_center_y, DOCSTR("[float] The y-coordinate of the center of this Aabr."));
    Py_Aabrf.def_property("bottom_left", &Aabrf::get_bottom_left, &Aabrf::set_bottom_left, DOCSTR("[V2f] The bottom-left corner of this Aabr."));
    Py_Aabrf.def_property("top_right", &Aabrf::get_top_right, &Aabrf::set_top_right, DOCSTR("[V2f] The top-right corner of this Aabr."));
    Py_Aabrf.def_property("top_left", &Aabrf::get_top_left, &Aabrf::set_top_left, DOCSTR("[V2f] The top-left corner of this Aabr."));
    Py_Aabrf.def_property("bottom_right", &Aabrf::get_bottom_right, &Aabrf::set_bottom_right, DOCSTR("[V2f] The bottom-right corner of this Aabr."));
    Py_Aabrf.def_property("width", &Aabrf::get_width, &Aabrf::set_width, DOCSTR("[float] The width of this Aabr."));
    Py_Aabrf.def_property("height", &Aabrf::get_height, &Aabrf::set_height, DOCSTR("[float] The height of this Aabr."));
    Py_Aabrf.def_property("size", &Aabrf::get_size, &Aabrf::set_size, DOCSTR("[Size2f] The size of this Aabr."));

    // inspection
    Py_Aabrf.def("is_zero", method_cast<Aabrf>(&Aabrf::is_zero), DOCSTR("Test, if this Aabr is null; The null Aabr has no area and is located at zero."), py::arg("epsilon") = precision_high<float>());
    Py_Aabrf.def("is_valid", &Aabrf::is_valid, DOCSTR("A valid Aabr has a width and height >= 0."));
    Py_Aabrf.def("contains", &Aabrf::contains, DOCSTR("Checks if this Aabr contains a given point."), py::arg("point"));
    Py_Aabrf.def("intersects", &Aabrf::intersects, DOCSTR("Checks if two Aabrs intersect."), py::arg("other"));
    Py_Aabrf.def("get_area", &Aabrf::get_area, DOCSTR("[float] The size of this Aabr."));
    Py_Aabrf.def("get_closest_point_to", &Aabrf::get_closest_point_to, DOCSTR("Returns the closest point inside the Aabr to a given target point."), py::arg("target"));
    Py_Aabrf.def("get_longer_side", &Aabrf::get_longer_side, DOCSTR("Returns the length of the longer side of this Aabr."));
    Py_Aabrf.def("get_shorter_side", &Aabrf::get_shorter_side, DOCSTR("Returns the length of the shorter side of this Aabr."));

    // modification
    Py_Aabrf.def("move_by", &Aabrf::move_by, DOCSTR("Moves this Aabr by a relative amount."), py::arg("delta"));
    Py_Aabrf.def("grow", &Aabrf::grow, DOCSTR("Moves each edge of the Aabr a given amount towards the outside."), py::arg("amount"));
    Py_Aabrf.def("grow_to", &Aabrf::grow_to, DOCSTR("Grows this Aabr to include the given point."), py::arg("point"));
    Py_Aabrf.def("shrink", &Aabrf::shrink, DOCSTR("Moves each edge of the Aabr a given amount towards the inside."), py::arg("amount"));
    Py_Aabrf.def("intersect", &Aabrf::intersect, DOCSTR("Intersects this Aabr with `other` in-place."), py::arg("other"));
    Py_Aabrf.def("unite", &Aabrf::unite, DOCSTR("Unites this Aabr with `other` in-place."), py::arg("other"));

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    // operators
    Py_Aabrf.def(py::self == py::self);
    Py_Aabrf.def(py::self != py::self);
    Py_Aabrf.def(py::self & py::self);
    Py_Aabrf.def(py::self &= py::self);
    Py_Aabrf.def(py::self | py::self);
    Py_Aabrf.def(py::self |= py::self);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    // builtins
    Py_Aabrf.def("__repr__", [](const Aabrf& aabr) {
        return fmt::format("pycnotf.Aabrf(min: ({}, {}) -> max: ({}, {}))", aabr.get_left(), aabr.get_bottom(),
                           aabr.get_right(), aabr.get_top());
    });
}

// clang-format on

