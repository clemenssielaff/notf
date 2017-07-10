#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/aabr.hpp"
#include "common/string.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_aabr(pybind11::module& module)
{
    py::class_<Aabrf> PyAabr(module, "Aabr");

    // constructors
    PyAabr.def(py::init<>());
    PyAabr.def(py::init<float, float>());
    PyAabr.def(py::init<float, float, float, float>());
    PyAabr.def(py::init<const Vector2f&, float, float>());
    PyAabr.def(py::init<const Vector2f&, const Vector2f&>());

    // static constructors
    PyAabr.def_static("null", &Aabrf::zero, "The null Aabr.");

    // properties
    PyAabr.def_property("x", &Aabrf::x, &Aabrf::set_x);
    PyAabr.def_property("y", &Aabrf::y, &Aabrf::set_y);
    PyAabr.def_property("center", &Aabrf::center, &Aabrf::set_center);
    PyAabr.def_property("left", &Aabrf::left, &Aabrf::set_left);
    PyAabr.def_property("right", &Aabrf::right, &Aabrf::set_right);
    PyAabr.def_property("top", &Aabrf::top, &Aabrf::set_top);
    PyAabr.def_property("bottom", &Aabrf::bottom, &Aabrf::set_bottom);
    PyAabr.def_property("top_left", &Aabrf::top_left, &Aabrf::set_top_left);
    PyAabr.def_property("top_right", &Aabrf::top_right, &Aabrf::set_top_right);
    PyAabr.def_property("bottom_left", &Aabrf::bottom_left, &Aabrf::set_bottom_left);
    PyAabr.def_property("bottom_right", &Aabrf::bottom_right, &Aabrf::set_bottom_right);
    PyAabr.def_property("width", &Aabrf::get_width, &Aabrf::set_width);
    PyAabr.def_property("height", &Aabrf::get_height, &Aabrf::set_height);
    PyAabr.def_property_readonly("area", &Aabrf::get_area);

    // inspections
    PyAabr.def("is_null", &Aabrf::is_zero, DOCSTR("Test, if this Aabr is null; The null Aabr has no area and is located at zero."));
    PyAabr.def("contains", &Aabrf::contains, DOCSTR("Checks if this Aabr contains a given point."), py::arg("point"));
    PyAabr.def("closest_point_to", &Aabrf::closest_point_to, DOCSTR("Returns the closest point inside the Aabr to a given target point."), py::arg("target"));

    // modification
    PyAabr.def("set_null", &Aabrf::set_zero, DOCSTR("Sets this Aabr to null."));
    PyAabr.def("grow", (Aabrf & (Aabrf::*)(float)) & Aabrf::grow, DOCSTR("Moves each edge of the Aabr a given amount towards the outside."), py::arg("amount"));
    PyAabr.def("shrink", (Aabrf & (Aabrf::*)(float)) & Aabrf::shrink, DOCSTR("Moves each edge of the Aabr a given amount towards the inside."), py::arg("amount"));
    PyAabr.def("intersect", (Aabrf & (Aabrf::*)(const Aabrf&)) & Aabrf::intersect, DOCSTR("Intersects this Aabr with `other` in-place."), py::arg("other"));
    PyAabr.def("unite", (Aabrf & (Aabrf::*)(const Aabrf&)) & Aabrf::unite, DOCSTR("Unites this Aabr with `other` in-place."), py::arg("other"));

    // operators
    PyAabr.def(py::self == py::self);
    PyAabr.def(py::self != py::self);
    PyAabr.def(py::self & py::self);
    PyAabr.def(py::self &= py::self);
    PyAabr.def(py::self | py::self);
    PyAabr.def(py::self |= py::self);

    // representation
    PyAabr.def("__repr__", [](const Aabrf& aabr) {
        return string_format("notf.Aabr([%f, %f], [%f, %f])", aabr.left(), aabr.top(), aabr.right(), aabr.bottom());
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
