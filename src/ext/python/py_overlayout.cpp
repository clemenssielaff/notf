#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "dynamic/layout/overlayout.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_overlayout(pybind11::module& module, py::detail::generic_type Py_LayoutItem)
{
    py::class_<Overlayout, std::shared_ptr<Overlayout>> Py_Overlayout(module, "Overlayout", Py_LayoutItem);

    Py_Overlayout.def(py::init<>());

    Py_Overlayout.def("get_padding", &Overlayout::get_padding, DOCSTR("Padding around the Layout's border."));
    Py_Overlayout.def("get_claim", &Overlayout::get_claim, DOCSTR("The current claim of this Item."));
    Py_Overlayout.def("get_horizontal_alignment", &Overlayout::get_horizontal_alignment, DOCSTR("Horizontal alignment of all items in the Layout."));
    Py_Overlayout.def("get_vertical_alignment", &Overlayout::get_vertical_alignment, DOCSTR("Vertical alignment of all items in the Layout."));

    Py_Overlayout.def("set_padding", &Overlayout::set_padding, DOCSTR("Sets the spacing between items."), py::arg("padding"));
    Py_Overlayout.def("set_claim", &Overlayout::set_claim, DOCSTR("Sets an explicit claim for this Layout."), py::arg("claim"));
    Py_Overlayout.def("set_alignment", &Overlayout::set_alignment, DOCSTR("Defines the alignment of each Item in the Layout."), py::arg("horizontal"), py::arg("vertical"));

    Py_Overlayout.def("add_item", &Overlayout::add_item, DOCSTR("Adds a new Item to the front of the Layout."), py::arg("item"));

    py::enum_<Overlayout::Horizontal>(Py_Overlayout, "Horizontal")
        .value("LEFT", Overlayout::Horizontal::LEFT)
        .value("CENTER", Overlayout::Horizontal::CENTER)
        .value("RIGHT", Overlayout::Horizontal::RIGHT);

    py::enum_<Overlayout::Vertical>(Py_Overlayout, "Vertical")
        .value("TOP", Overlayout::Vertical::TOP)
        .value("CENTER", Overlayout::Vertical::CENTER)
        .value("BOTTOM", Overlayout::Vertical::BOTTOM);
}
