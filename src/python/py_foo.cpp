#include "pybind11/pybind11.h"
namespace py = pybind11;

#include <assert.h>
#include <vector>

#include "common/log.hpp"
#include "core/foo.hpp"
using namespace notf;

class Python_Foo : public Foo {
public:
    using Foo::Foo;

    /* Trampoline (need one for each virtual function) */
    virtual void do_foo() const override
    {
        PYBIND11_OVERLOAD_PURE(
            void, /* Return type */
            Foo, /* Parent class */
            do_foo, /* Name of function */
            /* Argument(s) */
            );
    }
};

namespace {

static void (*foo_dealloc_orig)(PyObject*) = nullptr;
static void foo_dealloc(PyObject* object)
{
    auto instance = reinterpret_cast<py::detail::instance<Foo, std::shared_ptr<Foo>>*>(object);
    if (instance->holder.use_count() > 1) {

        instance->value->set_pyobject(object);
        assert(object->ob_refcnt == 1);
        instance->holder.reset();
    }
    else {
        assert(foo_dealloc_orig);
        foo_dealloc_orig(object);
    }
}
}

void produce_foo(pybind11::module& module)
{
    py::class_<Foo, std::shared_ptr<Foo>, Python_Foo> Py_Foo(module, "Foo");
    PyTypeObject* type = reinterpret_cast<PyTypeObject*>(Py_Foo.ptr());
    foo_dealloc_orig = type->tp_dealloc;
    type->tp_dealloc = foo_dealloc;

    Py_Foo.def(py::init<>());
    Py_Foo.def("do_foo", &Foo::do_foo);

    py::class_<Bar, std::shared_ptr<Bar>>(module, "Bar", Py_Foo).def(py::init<>());

    module.def("add_foo", &add_foo, py::arg("foo"));
    module.def("do_the_foos", &do_the_foos);
}
