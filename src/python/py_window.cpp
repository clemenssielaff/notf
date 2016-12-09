#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/layout_root.hpp"
#include "core/window.hpp"
#include "python/docstr.hpp"
using namespace notf;

void produce_window(pybind11::module& module)
{
    py::class_<Window, std::shared_ptr<Window>> Py_Window(module, "_Window");

    module.def("Window", []() -> std::shared_ptr<Window> {
        return Application::get_instance().get_current_window();
    }, DOCSTR("Reference to the current Window of the Application."));

    Py_Window.def("get_layout_root", &Window::get_layout_root, DOCSTR("The invisible root Layout of this Window."));
}
