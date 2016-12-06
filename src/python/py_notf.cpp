#include "python/py_notf.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/component.hpp"
#include "core/layout_item.hpp"
using namespace notf;

void produce_resource_manager(pybind11::module& module);
void produce_painter(pybind11::module& module);
void produce_vector2(pybind11::module& module);
void produce_size2i(pybind11::module& module);
void produce_size2f(pybind11::module& module);
void produce_aabr(pybind11::module& module);
void produce_padding(pybind11::module& module);
void produce_color(pybind11::module& module);
void produce_circle(pybind11::module& module);
void produce_claim(pybind11::module& module);
void produce_font(pybind11::module& module);
void produce_texture2(pybind11::module& module);
void produce_layout(pybind11::module& module);
void produce_window(pybind11::module& module);
void produce_state(pybind11::module& module);
void produce_widget(pybind11::module& module, py::detail::generic_type ancestor);
void produce_canvas_component(pybind11::module& module, py::detail::generic_type ancestor);
void produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor);
void produce_stack_layout(pybind11::module& module, py::detail::generic_type ancestor);
void produce_overlayout(pybind11::module& module, py::detail::generic_type ancestor);
py::class_<LayoutItem, std::shared_ptr<LayoutItem>> produce_layout_item(pybind11::module& module);
py::class_<Component, std::shared_ptr<Component>> produce_component(pybind11::module& module);
void produce_foo(pybind11::module& module); // TODO: test

const char* python_notf_module_name = "notf";

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, "NoTF Python bindings");
    produce_size2i(module);
    produce_size2f(module);
    produce_vector2(module);
    produce_aabr(module);
    produce_padding(module);
    produce_circle(module);
    produce_claim(module);
    produce_color(module);
    produce_state(module);
    produce_resource_manager(module);
    produce_texture2(module);
    produce_font(module);
    produce_painter(module);
    produce_window(module);
    produce_layout(module);
    produce_foo(module);

    py::class_<Component, std::shared_ptr<Component>> Py_Component(module, "_Component");
    produce_canvas_component(module, Py_Component);

    py::class_<LayoutItem, std::shared_ptr<LayoutItem>> Py_LayoutItem(module, "_LayoutItem");
    produce_widget(module, Py_LayoutItem);
    produce_layout_root(module, Py_LayoutItem);
    produce_stack_layout(module, Py_LayoutItem);
    produce_overlayout(module, Py_LayoutItem);

    return module.ptr();
}
