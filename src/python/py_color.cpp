#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/color.hpp"
#include "common/string.hpp"
#include "common/float.hpp"
#include "python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_color(pybind11::module& module)
{
    py::class_<Color> PyColor(module, "Color");

    // constructors
    PyColor.def(py::init<>());
    PyColor.def(py::init<int, int, int>());
    PyColor.def(py::init<int, int, int, int>());
    PyColor.def(py::init<const std::string&>());

    // static helper
    PyColor.def_static("is_color", &Color::is_color, DOCSTR("Checks, if the given string is a valid color value that can be passed to the constructor."), py::arg("value"));

    // static constructors
    PyColor.def_static("from_rgb", [](float r, float g, float b, float a = 1) -> Color {
        return Color::from_rgb(r, g, b, a);
    }, DOCSTR("Creates a Color from rgb(a) floats in the range [0, 1]"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 1.f);
    PyColor.def_static("from_rgb", [](int r, int g, int b, int a = 255) -> Color {
        return Color::from_rgb(r, g, b, a);
    }, DOCSTR("Creates a Color from rgb(a) integers in the range [0, 255]"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255);
    PyColor.def_static("from_hsl", [](float h, float s, float l, float a = 1) -> Color {
        return Color::from_hsl(h, s, l, a);
    }, DOCSTR("Creates a Color from hsl(a) floats in the range [0, 1]"), py::arg("h"), py::arg("s"), py::arg("l"), py::arg("a") = 1.f);

    // properties
    PyColor.def_readonly("r", &Color::r);
    PyColor.def_readonly("g", &Color::g);
    PyColor.def_readonly("b", &Color::b);
    PyColor.def_readonly("a", &Color::a);

    // inspections
    PyColor.def("to_string", &Color::to_string, DOCSTR("Returns the Color as an RGB string value."));
    PyColor.def("to_greyscale", &Color::to_greyscale, DOCSTR("Weighted conversion of this color to greyscale."));

    // operators
    PyColor.def(py::self == py::self);
    PyColor.def(py::self != py::self);

    // representation
    PyColor.def("__repr__", [](const Color& color) {
        return string_format("notf.Color(%i, %i, %i, %i)",
                             static_cast<int>(round(color.r * 255.f)),
                             static_cast<int>(round(color.g * 255.f)),
                             static_cast<int>(round(color.b * 255.f)),
                             static_cast<int>(round(color.a * 255.f)));
    });

    // free functions
    module.def("lerp", (Color(*)(const Color&, const Color&, float)) &lerp, DOCSTR("Linear interpolation between two Colors."), py::arg("from"), py::arg("to"), py::arg("blend"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
