#include "python/pynotf.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

#include "python/pycanvascomponent.hpp"
#include "python/pycomponent.hpp"
#include "python/pyglobal.hpp"
#include "python/pylayoutitem.hpp"
#include "python/pylayoutroot.hpp"
#include "python/pystacklayout.hpp"
#include "python/pyvector2.hpp"
#include "python/pywidget.hpp"
#include "python/pywindow.hpp"
using namespace notf;

const char* python_notf_module_name = "notf";

PyObject* produce_pynotf_module()
{
    py::module module(python_notf_module_name, "NoTF Python bindings");

    // common
    produce_globals(module);
    produce_vector2(module);

    // core
    produce_window(module);

    auto Py_Component = produce_component(module);
    produce_canvas_component(module, Py_Component);

    auto Py_LayoutItem = produce_layout_item(module);
    produce_widget(module, Py_LayoutItem);
    produce_layout_root(module, Py_LayoutItem);
    produce_stack_layout(module, Py_LayoutItem);

    return module.ptr();
}
