#include "pybind11/pybind11.h"
namespace py = pybind11;

#include <memory>
#include <vector>

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

void produce_foo(pybind11::module& module)
{
    py::class_<Foo, std::shared_ptr<Foo>, Python_Foo> Py_Foo(module, "Foo");

    Py_Foo.def(py::init<>());
    Py_Foo.def("do_foo", &Foo::do_foo);

    py::class_<Bar, std::shared_ptr<Bar>>(module, "Bar", Py_Foo).def(py::init<>());

    module.def("add_foo", &add_foo, py::arg("foo"));
    module.def("do_the_foos", &do_the_foos);
}
