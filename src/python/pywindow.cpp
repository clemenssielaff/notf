#include "python/pywindow.hpp"

#include "core/application.hpp"
#include "core/window.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<Window, std::shared_ptr<Window>> produce_window(pybind11::module& module)
{
    py::class_<Window, std::shared_ptr<Window>> Py_Window(module, "_Window");

    module.def("Window", []() -> std::shared_ptr<Window> {
        return Application::get_instance().get_current_window();
    }, "Reference to the current Window of the Application.");

    Py_Window.def("get_layout_root", &Window::get_layout_root, "The invisible root Layout of this Window.");

    return Py_Window;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
