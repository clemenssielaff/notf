#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/size2.hpp"
#include "common/size2.hpp"
#include "common/string.hpp"
#include "python/docstr.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_size2f(pybind11::module& module)
{
    py::class_<Size2f> PySize2f(module, "Size2f");

    // constructors
    PySize2f.def(py::init<>());
    PySize2f.def(py::init<float, float>());

    // static constructors
    PySize2f.def_static("from_size2i", &Size2f::from_size2i, py::arg("size2i"));

    // properties
    PySize2f.def_readwrite("width", &Size2f::width);
    PySize2f.def_readwrite("height", &Size2f::height);

    // inspections
    PySize2f.def("is_zero", &Size2f::is_zero, DOCSTR("Tests if a rectangle of this Size had zero area."));
    PySize2f.def("is_valid", &Size2f::is_valid, DOCSTR("Tests if this Size is valid (>=0) in both dimensions."));

    // operators
    PySize2f.def(py::self == py::self);
    PySize2f.def(py::self != py::self);

    // representation
    PySize2f.def("__repr__", [](const Size2f& size2f) {
        return string_format("notf.Size2f(%f, %f)", size2f.width, size2f.height);
    });
}

void produce_size2i(pybind11::module& module)
{
    py::class_<Size2i> PySize2i(module, "Size2i");

    // constructors
    PySize2i.def(py::init<>());
    PySize2i.def(py::init<float, float>());

    // static constructors
    PySize2i.def_static("from_size2f", &Size2i::from_size2f, py::arg("size2f"));

    // properties
    PySize2i.def_readwrite("width", &Size2i::width);
    PySize2i.def_readwrite("height", &Size2i::height);

    // inspections
    PySize2i.def("is_null", &Size2i::is_null, DOCSTR("Tests if this Size is null."));
    PySize2i.def("is_valid", &Size2i::is_valid, DOCSTR("Tests if this Size is valid (>=0) in both dimensions."));

    // operators
    PySize2i.def(py::self == py::self);
    PySize2i.def(py::self != py::self);

    // representation
    PySize2i.def("__repr__", [](const Size2i& size2i) {
        return string_format("notf.Size2i(%i, %i)", size2i.width, size2i.height);
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
