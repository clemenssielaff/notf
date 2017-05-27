#include "ext/python/py_notf.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/layout_item.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_resource_manager(pybind11::module& module);
//void produce_painter(pybind11::module& module);
void produce_vector2(pybind11::module& module);
void produce_size2i(pybind11::module& module);
void produce_size2f(pybind11::module& module);
void produce_aabr(pybind11::module& module);
void produce_padding(pybind11::module& module);
void produce_color(pybind11::module& module);
void produce_circle(pybind11::module& module);
void produce_claim(pybind11::module& module);
//void produce_font(pybind11::module& module);
//void produce_texture2(pybind11::module& module);
void produce_window(pybind11::module& module);
//void produce_widget(pybind11::module& module, py::detail::generic_type ancestor);
void produce_layout_root(pybind11::module& module, py::detail::generic_type ancestor);
void produce_flex_layout(pybind11::module& module, py::detail::generic_type ancestor);
void produce_overlayout(pybind11::module& module, py::detail::generic_type ancestor);
void produce_controller(pybind11::module& module, py::detail::generic_type Py_Item);
void produce_globals(pybind11::module& module);
void produce_events(pybind11::module& module);
void produce_signals(pybind11::module& module);
void produce_properties(pybind11::module& module);

const char* python_notf_module_name = "notf";

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, DOCSTR("NoTF Python bindings"));
    produce_globals(module);
    produce_events(module);
    produce_size2i(module);
    produce_size2f(module);
    produce_vector2(module);
    produce_aabr(module);
    produce_padding(module);
    produce_circle(module);
    produce_claim(module);
    produce_color(module);
    produce_resource_manager(module);
//    produce_texture2(module);
//    produce_font(module);
//    produce_painter(module);
    produce_window(module);
    produce_signals(module);
    produce_properties(module);

    py::class_<Item, ItemPtr> Py_Item(module, "_Item");
    produce_controller(module, Py_Item);

    py::class_<LayoutItem, std::shared_ptr<LayoutItem>> Py_LayoutItem(module, "_LayoutItem", Py_Item);
//    produce_widget(module, Py_LayoutItem);
    produce_layout_root(module, Py_LayoutItem);
    produce_flex_layout(module, Py_LayoutItem);
    produce_overlayout(module, Py_LayoutItem);

    return module.ptr();
}
