#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/components/canvas_component.hpp"
#include "graphics/painter.hpp"
using namespace notf;

void produce_canvas_component(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<CanvasComponent, std::shared_ptr<CanvasComponent>> Py_CanvasComponent(module, "_CanvasComponent", ancestor);

    module.def("CanvasComponent", []() -> std::shared_ptr<CanvasComponent> {
        return make_component<CanvasComponent>();
    }, "Creates a new CanvasComponent.");

    // TODO: it would be nice if we could inspect the method here and see if it fits the signature
    Py_CanvasComponent.def("set_paint_function", &CanvasComponent::set_paint_function, "Sets a new function for painting the canvas.", py::arg("paint_function"));
}
