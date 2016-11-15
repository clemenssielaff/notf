#include "python/pynotf.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

#include "core/component.hpp"
#include "core/layout_item.hpp"
using namespace notf;

void produce_globals(pybind11::module& module);
void produce_painter(pybind11::module& module);
void produce_vector2(pybind11::module& module);
void produce_aabr(pybind11::module& module);
void produce_window(pybind11::module& module);
void produce_color(pybind11::module& module);
void produce_widget(pybind11::module& module, py::detail::generic_type ancestor);
void produce_canvas_component(pybind11::module& module, py::detail::generic_type ancestor);
void produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor);
void produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor);
py::class_<LayoutItem, std::shared_ptr<LayoutItem>> produce_layout_item(pybind11::module& module);
py::class_<Component, std::shared_ptr<Component>> produce_component(pybind11::module& module);

const char* python_notf_module_name = "notf";

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, "NoTF Python bindings");
    produce_globals(module);
    produce_vector2(module);
    produce_aabr(module);
    produce_color(module);
    produce_painter(module);
    produce_window(module);

    py::class_<Component, std::shared_ptr<Component>> Py_Component(module, "_Component");
    produce_canvas_component(module, Py_Component);

    py::class_<LayoutItem, std::shared_ptr<LayoutItem>> Py_LayoutItem(module, "_LayoutItem");
    produce_widget(module, Py_LayoutItem);
    produce_layout_root(module, Py_LayoutItem);
    produce_stack_layout(module, Py_LayoutItem);

    return module.ptr();
}
