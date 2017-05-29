#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/padding.hpp"
#include "common/string.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_padding(pybind11::module& module)
{
    py::class_<Padding> PyPadding(module, "Padding");

    // constructors
    PyPadding.def("__init__", [](Padding& self) {
        new (&self) Padding{0.f, 0.f, 0.f, 0.f};
    });
    PyPadding.def("__init__", [](Padding& self, float top, float right, float bottom, float left) {
        new (&self) Padding{top, right, bottom, left};
    }, py::arg("top"), py::arg("right"), py::arg("bottom"), py::arg("left"));

    // static constructors
    PyPadding.def_static("all", &Padding::all, DOCSTR("Even padding on all sides."), py::arg("padding"));
    PyPadding.def_static("none", &Padding::none, DOCSTR("No padding."));
    PyPadding.def_static("horizontal", &Padding::horizontal, DOCSTR("Horizontal padding, sets both `left` and `right`."), py::arg("padding"));
    PyPadding.def_static("vertical", &Padding::vertical, DOCSTR("AlignVertical padding, sets both `top` and `bottom`."), py::arg("padding"));

    // properties
    PyPadding.def_readonly("top", &Padding::top);
    PyPadding.def_readonly("right", &Padding::right);
    PyPadding.def_readonly("bottom", &Padding::bottom);
    PyPadding.def_readonly("left", &Padding::left);

    // inspections
    PyPadding.def("is_padding", &Padding::is_padding, DOCSTR("Checks if any of the sides is padding."));
    PyPadding.def("is_valid", &Padding::is_valid, DOCSTR("Checks if this Padding is valid (all sides have values >= 0)."));

    // representation
    PyPadding.def("__repr__", [](const Padding& padding) {
        return string_format("notf.Padding(top = %f, right = %f, bottom = %f, left = %f)", padding.top, padding.right, padding.bottom, padding.left);
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
