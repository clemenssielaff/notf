#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "dynamic/layout/flex_layout.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_flex_layout(pybind11::module& module, py::detail::generic_type Py_LayoutItem)
{
    py::class_<FlexLayout, std::shared_ptr<FlexLayout>> Py_FlexLayout(module, "FlexLayout", Py_LayoutItem);

    Py_FlexLayout.def(py::init<const FlexLayout::Direction>());

    Py_FlexLayout.def("get_direction", &FlexLayout::get_direction, DOCSTR("Direction in which items are stacked."));
    Py_FlexLayout.def("get_alignment", &FlexLayout::get_alignment, DOCSTR("Alignment of items in the main direction."));
    Py_FlexLayout.def("get_cross_alignment", &FlexLayout::get_cross_alignment, DOCSTR("Alignment of items in the cross direction."));
    Py_FlexLayout.def("get_content_alignment", &FlexLayout::get_content_alignment, DOCSTR("Cross alignment the entire content if the Layout wraps."));
    Py_FlexLayout.def("get_wrap", &FlexLayout::get_wrap, DOCSTR("How (and if) overflowing lines are wrapped."));
    Py_FlexLayout.def("is_wrapping", &FlexLayout::is_wrapping, DOCSTR("True if overflowing lines are wrapped."));
    Py_FlexLayout.def("get_padding", &FlexLayout::get_padding, DOCSTR("Padding around the Layout's border."));
    Py_FlexLayout.def("get_spacing", &FlexLayout::get_spacing, DOCSTR("Spacing between items."));
    Py_FlexLayout.def("get_cross_spacing", &FlexLayout::get_cross_spacing, DOCSTR("Spacing between stacks of items if this Layout is wrapped."));

    Py_FlexLayout.def("set_direction", &FlexLayout::set_spacing, DOCSTR("Sets the spacing between items."), py::arg("spacing"));
    Py_FlexLayout.def("set_alignment", &FlexLayout::set_alignment, DOCSTR("Sets the alignment of items in the main direction."), py::arg("alignment"));
    Py_FlexLayout.def("set_cross_alignment", &FlexLayout::set_cross_alignment, DOCSTR("Sets the alignment of items in the cross direction."), py::arg("alignment"));
    Py_FlexLayout.def("set_content_alignment", &FlexLayout::set_content_alignment, DOCSTR("Defines the cross alignment the entire content if the Layout wraps."), py::arg("alignment"));
    Py_FlexLayout.def("set_wrap", &FlexLayout::set_wrap, DOCSTR("Defines how (and if) overflowing lines are wrapped."), py::arg("wrap"));
    Py_FlexLayout.def("set_padding", &FlexLayout::set_padding, DOCSTR("Sets the spacing between items."), py::arg("padding"));
    Py_FlexLayout.def("set_spacing", &FlexLayout::set_spacing, DOCSTR("Sets the spacing between items."), py::arg("spacing"));
    Py_FlexLayout.def("set_cross_spacing", &FlexLayout::set_cross_spacing, DOCSTR("Defines the spacing between stacks of items if this Layout is wrapped."), py::arg("spacing"));

    Py_FlexLayout.def("add_item", &FlexLayout::add_item, DOCSTR("Adds a new Item into the Layout."), py::arg("item"));

    py::enum_<FlexLayout::Direction>(Py_FlexLayout, "Direction")
        .value("LEFT_TO_RIGHT", FlexLayout::Direction::LEFT_TO_RIGHT)
        .value("TOP_TO_BOTTOM", FlexLayout::Direction::TOP_TO_BOTTOM)
        .value("RIGHT_TO_LEFT", FlexLayout::Direction::RIGHT_TO_LEFT)
        .value("BOTTOM_TO_TOP", FlexLayout::Direction::BOTTOM_TO_TOP);

    py::enum_<FlexLayout::Alignment>(Py_FlexLayout, "Alignment")
        .value("START", FlexLayout::Alignment::START)
        .value("END", FlexLayout::Alignment::END)
        .value("CENTER", FlexLayout::Alignment::CENTER)
        .value("SPACE_BETWEEN", FlexLayout::Alignment::SPACE_BETWEEN)
        .value("SPACE_AROUND", FlexLayout::Alignment::SPACE_AROUND)
        .value("SPACE_EQUAL", FlexLayout::Alignment::SPACE_EQUAL);

    py::enum_<FlexLayout::Wrap>(Py_FlexLayout, "Wrap")
        .value("NO_WRAP", FlexLayout::Wrap::NO_WRAP)
        .value("WRAP", FlexLayout::Wrap::WRAP)
        .value("WRAP_REVERSE", FlexLayout::Wrap::REVERSE);
}
