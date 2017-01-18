#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "dynamic/layout/stack_layout.hpp"
#include "python/docstr.hpp"
using namespace notf;

void produce_stack_layout(pybind11::module& module, py::detail::generic_type Py_LayoutItem)
{
    py::class_<StackLayout, std::shared_ptr<StackLayout>> Py_StackLayout(module, "StackLayout", Py_LayoutItem);

    Py_StackLayout.def(py::init<const StackLayout::Direction>());

    Py_StackLayout.def("get_direction", &StackLayout::get_direction, DOCSTR("Direction in which items are stacked."));
    Py_StackLayout.def("get_alignment", &StackLayout::get_alignment, DOCSTR("Alignment of items in the main direction."));
    Py_StackLayout.def("get_cross_alignment", &StackLayout::get_cross_alignment, DOCSTR("Alignment of items in the cross direction."));
    Py_StackLayout.def("get_content_alignment", &StackLayout::get_content_alignment, DOCSTR("Cross alignment the entire content if the Layout wraps."));
    Py_StackLayout.def("get_wrap", &StackLayout::get_wrap, DOCSTR("How (and if) overflowing lines are wrapped."));
    Py_StackLayout.def("is_wrapping", &StackLayout::is_wrapping, DOCSTR("True if overflowing lines are wrapped."));
    Py_StackLayout.def("get_padding", &StackLayout::get_padding, DOCSTR("Padding around the Layout's border."));
    Py_StackLayout.def("get_spacing", &StackLayout::get_spacing, DOCSTR("Spacing between items."));
    Py_StackLayout.def("get_cross_spacing", &StackLayout::get_cross_spacing, DOCSTR("Spacing between stacks of items if this Layout is wrapped."));

    Py_StackLayout.def("set_direction", &StackLayout::set_spacing, DOCSTR("Sets the spacing between items."), py::arg("spacing"));
    Py_StackLayout.def("set_alignment", &StackLayout::set_alignment, DOCSTR("Sets the alignment of items in the main direction."), py::arg("alignment"));
    Py_StackLayout.def("set_cross_alignment", &StackLayout::set_cross_alignment, DOCSTR("Sets the alignment of items in the cross direction."), py::arg("alignment"));
    Py_StackLayout.def("set_content_alignment", &StackLayout::set_content_alignment, DOCSTR("Defines the cross alignment the entire content if the Layout wraps."), py::arg("alignment"));
    Py_StackLayout.def("set_wrap", &StackLayout::set_wrap, DOCSTR("Defines how (and if) overflowing lines are wrapped."), py::arg("wrap"));
    Py_StackLayout.def("set_padding", &StackLayout::set_padding, DOCSTR("Sets the spacing between items."), py::arg("padding"));
    Py_StackLayout.def("set_spacing", &StackLayout::set_spacing, DOCSTR("Sets the spacing between items."), py::arg("spacing"));
    Py_StackLayout.def("set_cross_spacing", &StackLayout::set_cross_spacing, DOCSTR("Defines the spacing between stacks of items if this Layout is wrapped."), py::arg("spacing"));

    Py_StackLayout.def("add_item", &StackLayout::add_item, DOCSTR("Adds a new Item into the Layout."), py::arg("item"));

    py::enum_<StackLayout::Direction>(Py_StackLayout, "Direction")
        .value("LEFT_TO_RIGHT", StackLayout::Direction::LEFT_TO_RIGHT)
        .value("TOP_TO_BOTTOM", StackLayout::Direction::TOP_TO_BOTTOM)
        .value("RIGHT_TO_LEFT", StackLayout::Direction::RIGHT_TO_LEFT)
        .value("BOTTOM_TO_TOP", StackLayout::Direction::BOTTOM_TO_TOP);

    py::enum_<StackLayout::Alignment>(Py_StackLayout, "Alignment")
        .value("START", StackLayout::Alignment::START)
        .value("END", StackLayout::Alignment::END)
        .value("CENTER", StackLayout::Alignment::CENTER)
        .value("SPACE_BETWEEN", StackLayout::Alignment::SPACE_BETWEEN)
        .value("SPACE_AROUND", StackLayout::Alignment::SPACE_AROUND)
        .value("SPACE_EQUAL", StackLayout::Alignment::SPACE_EQUAL);

    py::enum_<StackLayout::Wrap>(Py_StackLayout, "Wrap")
        .value("NO_WRAP", StackLayout::Wrap::NO_WRAP)
        .value("WRAP", StackLayout::Wrap::WRAP)
        .value("WRAP_REVERSE", StackLayout::Wrap::WRAP_REVERSE);
}
