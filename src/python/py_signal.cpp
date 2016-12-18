#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/events/mouse_event.hpp"
#include "python/docstr.hpp"
#include "python/py_signal.hpp"
using namespace notf;

void produce_signals(pybind11::module& module)
{
    py::class_<PySignal<MouseEvent&>>(module, "_Signal_MouseEventRef")
        .def("connect", &PySignal<MouseEvent&>::connect, DOCSTR("Connects a new target to this Signal."), py::arg("callback"), py::arg("test"));
}
