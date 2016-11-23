#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/layout.hpp"
using namespace notf;

namespace { // anonymous

/**
 * Object acting as the namespace for layout-specific enums.
 */
struct LayoutNamespace {
};

} // namespace  anonymous

void produce_layout(pybind11::module& module)
{
    py::class_<LayoutNamespace> Py_LayoutNamespace(module, "Layout");

    py::enum_<Layout::Direction>(Py_LayoutNamespace, "Direction")
        .value("LEFT_TO_RIGHT", Layout::Direction::LEFT_TO_RIGHT)
        .value("TOP_TO_BOTTOM", Layout::Direction::TOP_TO_BOTTOM)
        .value("RIGHT_TO_LEFT", Layout::Direction::RIGHT_TO_LEFT)
        .value("BOTTOM_TO_TOP", Layout::Direction::BOTTOM_TO_TOP);

    py::enum_<Layout::Alignment>(Py_LayoutNamespace, "Alignment")
        .value("START", Layout::Alignment::START)
        .value("END", Layout::Alignment::END)
        .value("CENTER", Layout::Alignment::CENTER)
        .value("SPACE_BETWEEN", Layout::Alignment::SPACE_BETWEEN)
        .value("SPACE_AROUND", Layout::Alignment::SPACE_AROUND);

    py::enum_<Layout::Wrap>(Py_LayoutNamespace, "Wrap")
        .value("NONE", Layout::Wrap::NONE)
        .value("WRAP", Layout::Wrap::WRAP)
        .value("REVERSE", Layout::Wrap::REVERSE);
}
