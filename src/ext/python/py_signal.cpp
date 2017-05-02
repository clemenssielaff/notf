#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/events/mouse_event.hpp"
#include "ext/python/docstr.hpp"
#include "ext/python/py_signal.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

namespace notf {
namespace detail {
    const char* s_signal_cache_name = "signal_handlers";
}
}

#pragma push_macro("DEFINE_PYSIGNAL")
#define define_pysignal(MODULE, TYPE, NAME)                                                                                                                                \
    py::class_<PySignal<TYPE>>(MODULE, NAME)                                                                                                                               \
        .def("connect", &PySignal<TYPE>::connect, DOCSTR("Connects a new target to this Signal."), py::arg("callback"), py::arg("test"))                                   \
        .def("has_connection", &PySignal<TYPE>::has_connection, DOCSTR("Checks if a particular Connection is connected to this Signal."), py::arg("id"))                   \
        .def("get_connections", &PySignal<TYPE>::get_connections, DOCSTR("Returns all Connections."))                                                                      \
        .def("disable", (void (PySignal<TYPE>::*)()) & PySignal<TYPE>::disable, DOCSTR("Disables all Connections of this Signal."))                                        \
        .def("disable", (void (PySignal<TYPE>::*)(const ConnectionID)) & PySignal<TYPE>::disable, DOCSTR("Disables a specific Connection of this Signal."), py::arg("id")) \
        .def("enable", (void (PySignal<TYPE>::*)()) & PySignal<TYPE>::enable, DOCSTR("Enables all Connections of this Signal."))                                           \
        .def("enable", (void (PySignal<TYPE>::*)(const ConnectionID)) & PySignal<TYPE>::enable, DOCSTR("Enables a specific Connection of this Signal."), py::arg("id"))    \
        .def("disconnect", (void (PySignal<TYPE>::*)()) & PySignal<TYPE>::disconnect, DOCSTR("Disconnects all Connections of this Signal."))                               \
        .def("disconnect", (void (PySignal<TYPE>::*)(const ConnectionID)) & PySignal<TYPE>::disconnect, DOCSTR("Disconnects a specific Connection of this Signal."), py::arg("id"))

void produce_signals(pybind11::module& module)
{
    define_pysignal(module, , "_Signal_Void");
    define_pysignal(module, MouseEvent&, "_Signal_MouseEventRef");
}

#undef DEFINE_PYSIGNAL
#pragma pop_macro("DEFINE_PYSIGNAL")

#ifdef __clang__
#pragma clang diagnostic pop
#endif
