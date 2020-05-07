#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "notf/common/geo/size2.hpp"
#include "docstr.hpp"
using namespace notf;

// size2f =========================================================================================================== //

void produce_size2f(pybind11::module& module)
{
    py::class_<Size2f> Py_Size2f(module, "Size2f");

    // constructors
    Py_Size2f.def(py::init<>());
    Py_Size2f.def(py::init<float, float>(), py::arg("width"), py::arg("height"));
    Py_Size2f.def(py::init<Size2i>(), py::arg("other"));

    // static constructors
    Py_Size2f.def_static("zero", &Size2f::zero, DOCSTR("The null Size2."));
    Py_Size2f.def_static("invalid", &Size2f::invalid, DOCSTR("Returns an invalid Size2 instance."));
    Py_Size2f.def_static("largest", &Size2f::largest, DOCSTR("The largest representable Size2."));
    Py_Size2f.def_static("wrongest", &Size2f::wrongest, DOCSTR("The 'most wrong' Size2 (maximal negative area)."));

    // properties
    Py_Size2f.def_property("width", &Size2f::get_width, &Size2f::set_width, DOCSTR("[float] The width."));
    Py_Size2f.def_property("height", &Size2f::get_height, &Size2f::set_height, DOCSTR("[float] The height."));

    // inspection
    Py_Size2f.def("is_zero", &Size2f::is_zero, DOCSTR("Tests if a rectangle of this Size had zero area."), py::arg("epsilon") = precision_high<float>());
    Py_Size2f.def("is_valid", &Size2f::is_valid, DOCSTR("Tests if this Size2 is valid (>=0) in both dimensions."));
    Py_Size2f.def("is_square", &Size2f::is_square, DOCSTR("Checks if the Size2 has the same height and width."));
    Py_Size2f.def("get_area", &Size2f::get_area, DOCSTR("Returns the area of a rectangle of this Size2 or 0 if invalid."));

    // operators
    Py_Size2f.def(py::self == py::self);
    Py_Size2f.def(py::self != py::self);

    // builtins
    Py_Size2f.def("__repr__", [](const Size2f& size2) {
        return fmt::format("pycnotf.Size2f(width: {} x height: {})", size2.get_width(), size2.get_height());
    });
}

// size2i =========================================================================================================== //

void produce_size2i(pybind11::module& module)
{
    py::class_<Size2i> Py_Size2i(module, "Size2i");

    // constructors
    Py_Size2i.def(py::init<>());
    Py_Size2i.def(py::init<float, float>(), py::arg("width"), py::arg("height"));
    Py_Size2i.def(py::init<Size2f>(), py::arg("other"));

    // static constructors
    Py_Size2i.def_static("zero", &Size2i::zero, DOCSTR("The null Size2."));
    Py_Size2i.def_static("invalid", &Size2i::invalid, DOCSTR("Returns an invalid Size2 instance."));
    Py_Size2i.def_static("largest", &Size2i::largest, DOCSTR("The largest representable Size2."));
    Py_Size2i.def_static("wrongest", &Size2i::wrongest, DOCSTR("The 'most wrong' Size2 (maximal negative area)."));

    // properties
    Py_Size2i.def_property("width", &Size2i::get_width, &Size2i::set_width, DOCSTR("[int] The width."));
    Py_Size2i.def_property("height", &Size2i::get_height, &Size2i::set_height, DOCSTR("[int] The height."));

    // inspection
    Py_Size2i.def("is_zero", &Size2i::is_zero, DOCSTR("Tests if a rectangle of this Size had zero area."), py::arg("epsilon") = precision_high<float>());
    Py_Size2i.def("is_valid", &Size2i::is_valid, DOCSTR("Tests if this Size2 is valid (>=0) in both dimensions."));
    Py_Size2i.def("is_square", &Size2i::is_square, DOCSTR("Checks if the Size2 has the same height and width."));
    Py_Size2i.def("get_area", &Size2i::get_area, DOCSTR("Returns the area of a rectangle of this Size2 or 0 if invalid."));

    // operators
    Py_Size2i.def(py::self == py::self);
    Py_Size2i.def(py::self != py::self);

    // builtins
    Py_Size2i.def("__repr__", [](const Size2i& size2) {
        return fmt::format("pycnotf.Size2i(width: {} x height: {})", size2.get_width(), size2.get_height());
    });
}

