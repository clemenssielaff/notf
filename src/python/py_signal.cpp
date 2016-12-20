#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/events/mouse_event.hpp"
#include "python/docstr.hpp"
#include "python/py_signal.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_signals(pybind11::module& module)
{
    py::class_<PySignal<MouseEvent&>>(module, "_Signal_MouseEventRef")
        .def("connect", &PySignal<MouseEvent&>::connect, DOCSTR("Connects a new target to this Signal."), py::arg("callback"), py::arg("test"))
        .def("has_connection", &PySignal<MouseEvent&>::has_connection, DOCSTR("Checks if a particular Connection is connected to this Signal."), py::arg("id"))
        .def("get_connections", &PySignal<MouseEvent&>::get_connections, DOCSTR("Returns all Connections."))
        .def("disable", (void (PySignal<MouseEvent&>::*)()) & PySignal<MouseEvent&>::disable, DOCSTR("Disables all Connections of this Signal."))
        .def("disable", (void (PySignal<MouseEvent&>::*)(const ConnectionID)) & PySignal<MouseEvent&>::disable, DOCSTR("Disables a specific Connection of this Signal."), py::arg("id"))
        .def("enable", (void (PySignal<MouseEvent&>::*)()) & PySignal<MouseEvent&>::enable, DOCSTR("Enables all Connections of this Signal."))
        .def("enable", (void (PySignal<MouseEvent&>::*)(const ConnectionID)) & PySignal<MouseEvent&>::enable, DOCSTR("Enables a specific Connection of this Signal."), py::arg("id"))
        .def("disconnect", (void (PySignal<MouseEvent&>::*)()) & PySignal<MouseEvent&>::disconnect, DOCSTR("Disconnects all Connections of this Signal."))
        .def("disconnect", (void (PySignal<MouseEvent&>::*)(const ConnectionID)) & PySignal<MouseEvent&>::disconnect, DOCSTR("Disconnects a specific Connection of this Signal."), py::arg("id"));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
