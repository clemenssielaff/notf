#include "python/pycanvascomponent.hpp"

#include "core/components/canvas_component.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<CanvasComponent, std::shared_ptr<CanvasComponent>> produce_canvas_component(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<CanvasComponent, std::shared_ptr<CanvasComponent>> Py_CanvasComponent (module, "_CanvasComponent", ancestor);

    module.def("CanvasComponent", []() -> std::shared_ptr<CanvasComponent> {
        return make_component<CanvasComponent>();
    }, "Creates a new CanvasComponent.");

    return Py_CanvasComponent;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
