#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/aabr.hpp"
#include "common/string_utils.hpp"
#include "python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_aabr(pybind11::module& module)
{
    py::class_<Aabr> PyAabr(module, "Aabr");

    // constructors
    PyAabr.def(py::init<>());
    PyAabr.def(py::init<float, float>());
    PyAabr.def(py::init<float, float, float, float>());
    PyAabr.def(py::init<const Vector2&, float, float>());
    PyAabr.def(py::init<const Vector2&, const Vector2&>());

    // static constructors
    PyAabr.def_static("null", &Aabr::null, "The null Aabr.");

    // properties
    PyAabr.def_property("x", &Aabr::x, &Aabr::set_x);
    PyAabr.def_property("y", &Aabr::y, &Aabr::set_y);
    PyAabr.def_property("center", &Aabr::center, &Aabr::set_center);
    PyAabr.def_property("left", &Aabr::left, &Aabr::set_left);
    PyAabr.def_property("right", &Aabr::right, &Aabr::set_right);
    PyAabr.def_property("top", &Aabr::top, &Aabr::set_top);
    PyAabr.def_property("bottom", &Aabr::bottom, &Aabr::set_bottom);
    PyAabr.def_property("top_left", &Aabr::top_left, &Aabr::set_top_left);
    PyAabr.def_property("top_right", &Aabr::top_right, &Aabr::set_top_right);
    PyAabr.def_property("bottom_left", &Aabr::bottom_left, &Aabr::set_bottom_left);
    PyAabr.def_property("bottom_right", &Aabr::bottom_right, &Aabr::set_bottom_right);
    PyAabr.def_property("width", &Aabr::width, &Aabr::set_width);
    PyAabr.def_property("height", &Aabr::height, &Aabr::set_height);
    PyAabr.def_property_readonly("area", &Aabr::area);

    // inspections
    PyAabr.def("is_null", &Aabr::is_null, DOCSTR("Test, if this Aabr is null; The null Aabr has no area and is located at zero."));
    PyAabr.def("contains", &Aabr::contains, DOCSTR("Checks if this Aabr contains a given point."), py::arg("point"));
    PyAabr.def("closest_point_to", &Aabr::closest_point_to, DOCSTR("Returns the closest point inside the Aabr to a given target point."), py::arg("target"));

    // modification
    PyAabr.def("set_null", &Aabr::set_null, DOCSTR("Sets this Aabr to null."));
    PyAabr.def("grow", &Aabr::grow, DOCSTR("Moves each edge of the Aabr a given amount towards the outside."), py::arg("amount"));
    PyAabr.def("shrink", &Aabr::shrink, DOCSTR("Moves each edge of the Aabr a given amount towards the inside."), py::arg("amount"));
    PyAabr.def("shrink", &Aabr::shrink, DOCSTR("Moves each edge of the Aabr a given amount towards the inside."), py::arg("amount"));
    PyAabr.def("intersection", &Aabr::intersection, DOCSTR("Intersection of this Aabr with `other`."), py::arg("other"));
    PyAabr.def("intersected", &Aabr::intersected, DOCSTR("Intersection of this Aabr with `other` in-place."), py::arg("other"));
    PyAabr.def("union", &Aabr::union_, DOCSTR("Creates the union of this Aabr with `other`."), py::arg("other"));
    PyAabr.def("united", &Aabr::united, DOCSTR("Creates the union of this Aabr with `other` in-place."), py::arg("other"));

    // operators
    PyAabr.def(py::self == py::self);
    PyAabr.def(py::self != py::self);
    PyAabr.def(py::self & py::self);
    PyAabr.def(py::self &= py::self);
    PyAabr.def(py::self | py::self);
    PyAabr.def(py::self |= py::self);

    // representation
    PyAabr.def("__repr__", [](const Aabr& aabr) {
        return string_format("notf.Aabr([%f, %f], [%f, %f])", aabr.left(), aabr.top(), aabr.right(), aabr.bottom());
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
