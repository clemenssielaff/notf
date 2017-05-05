#include <map>

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_events(pybind11::module& module)
{
    {
        py::class_<KeyEvent> Py_KeyEvent(module, "KeyEvent");

        Py_KeyEvent.def("was_handled", &KeyEvent::was_handled, DOCSTR("Checks whether this event was already handled or not."));
        Py_KeyEvent.def("set_handled", &KeyEvent::set_handled, DOCSTR("Must be called after a event handler handled this event."));

        Py_KeyEvent.def_readonly("key", &KeyEvent::key);
        Py_KeyEvent.def_readonly("action", &KeyEvent::action);
        Py_KeyEvent.def_readonly("modifiers", &KeyEvent::modifiers);
    }

    {
        py::class_<MouseEvent> Py_MouseEvent(module, "MouseEvent");

        Py_MouseEvent.def("was_handled", &MouseEvent::was_handled, DOCSTR("Checks whether this event was already handled or not."));
        Py_MouseEvent.def("set_handled", &MouseEvent::set_handled, DOCSTR("Must be called after a event handler handled this event."));

        Py_MouseEvent.def_readonly("window_pos", &MouseEvent::window_pos);
        Py_MouseEvent.def_readonly("window_delta", &MouseEvent::window_delta);
        Py_MouseEvent.def_readonly("button", &MouseEvent::button);
        Py_MouseEvent.def_readonly("action", &MouseEvent::action);
        Py_MouseEvent.def_readonly("modifiers", &MouseEvent::modifiers);
    }
}
