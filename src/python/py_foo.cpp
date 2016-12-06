#include "pybind11/pybind11.h"
namespace py = pybind11;

#include <vector>

#include "core/foo.hpp"
#include "python/python_ptr.hpp"
using namespace notf;

PYBIND11_DECLARE_HOLDER_TYPE(T, python_ptr<T>);

#define TEST_PTR python_ptr


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

void produce_foo(pybind11::module& module)
{
    py::class_<Foo, TEST_PTR<Foo>, Python_Foo> Py_Foo(module, "Foo");

    Py_Foo.def(py::init<>());
    Py_Foo.def("do_foo", &Foo::do_foo);

    py::class_<Bar, TEST_PTR<Bar>>(module, "Bar", Py_Foo).def(py::init<>());

    module.def("add_foo", &add_foo, py::arg("foo"));
    module.def("do_the_foos", &do_the_foos);
}
