#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/property_impl.hpp"
#include "core/state.hpp"

#define NOTF_BINDINGS
#include "core/widget.hpp"
using namespace notf;


/* Trampoline Class ***************************************************************************************************/

class WidgetTrampoline : public Widget {
public:
    using Widget::Widget;

//    /* Trampoline (need one for each virtual function) */
//    virtual void do_foo() const override
//    {
//        PYBIND11_OVERLOAD_PURE(
//            void, /* Return type */
//            Foo, /* Parent class */
//            do_foo, /* Name of function */
//            /* Argument(s) */
//            );
//    }
};

/* Custom Deallocator *************************************************************************************************/

static void (*py_widget_dealloc_orig)(PyObject*) = nullptr;
static void py_widget_dealloc(PyObject* object)
{
    auto instance = reinterpret_cast<py::detail::instance<Widget, std::shared_ptr<Widget>>*>(object);
    if (instance->holder.use_count() > 1) {

        instance->value->set_pyobject(object);
        assert(object->ob_refcnt == 1);
        instance->holder.reset();
    }
    else {
        assert(py_widget_dealloc_orig);
        py_widget_dealloc_orig(object);
    }
}

/* Bindings ***********************************************************************************************************/

void produce_widget(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<Widget, std::shared_ptr<Widget>> Py_Widget(module, "Widget", ancestor);
    PyTypeObject* py_widget_type = reinterpret_cast<PyTypeObject*>(Py_Widget.ptr());
    py_widget_dealloc_orig = py_widget_type->tp_dealloc;
    py_widget_type->tp_dealloc = py_widget_dealloc;

    Py_Widget.def(py::init<std::shared_ptr<StateMachine>>());

    Py_Widget.def("get_id", &Widget::get_id, "The application-unique ID of this Widget.");
}
