#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "docstr.hpp"
#include "notf/common/color.hpp"
using namespace notf;

void produce_color(pybind11::module& module) {
    py::class_<Color> PyColor(module, "Color");

    // constructors
    PyColor.def(py::init<>());
    PyColor.def(py::init<float, float, float, float>(), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 1.f);
    PyColor.def(py::init<int, int, int, int>(), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255);
    PyColor.def(py::init<const std::string&>());

    // static constructors
    PyColor.def_static("from_hsl", &Color::from_hsl, DOCSTR("Creates a Color from hsl(a) floats in the range [0, 1]"),
                       py::arg("h"), py::arg("s"), py::arg("l"), py::arg("a") = 1.f);
    PyColor.def_static("transparent", &Color::transparent, DOCSTR("Transparent color"));
    PyColor.def_static("black", &Color::black, DOCSTR("Black color"));
    PyColor.def_static("white", &Color::white, DOCSTR("White color"));
    PyColor.def_static("grey", &Color::grey, DOCSTR("Grey color"));
    PyColor.def_static("red", &Color::red, DOCSTR("Red color"));
    PyColor.def_static("green", &Color::green, DOCSTR("Green color"));
    PyColor.def_static("blue", &Color::blue, DOCSTR("Blue color"));

    // static methods
    PyColor.def_static(
        "is_color", &Color::is_color,
        DOCSTR("Checks, if the given string is a valid color value that can be passed to the constructor."),
        py::arg("string"));

    // properties
    PyColor.def_readonly("r", &Color::r);
    PyColor.def_readonly("g", &Color::g);
    PyColor.def_readonly("b", &Color::b);
    PyColor.def_readonly("a", &Color::a);

    // inspections
    PyColor.def("to_string", &Color::to_string, DOCSTR("Returns the Color as an RGB string value."));
    PyColor.def("to_greyscale", &Color::to_greyscale, DOCSTR("Weighted conversion of this color to greyscale."));
    PyColor.def("premultiplied", &Color::premultiplied, DOCSTR("Premultiplied copy of this Color."));

    // operators
    PyColor.def(py::self == py::self);
    PyColor.def(py::self != py::self);

    // representation
    PyColor.def("__repr__", [](const Color& color) {
        return fmt::format("notf.Color({}, {}, {}, {})", static_cast<int>(round(color.r * 255.f)),
                           static_cast<int>(round(color.g * 255.f)), static_cast<int>(round(color.b * 255.f)),
                           static_cast<int>(round(color.a * 255.f)));
    });

    // free functions
    module.def("lerp", (Color(*)(const Color&, const Color&, float)) & lerp,
               DOCSTR("Linear interpolation between two Colors."), py::arg("left"), py::arg("right"), py::arg("blend"));
}
