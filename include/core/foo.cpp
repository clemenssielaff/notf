#include "core/foo.hpp"

#include "common/log.hpp"

#include "pybind11/pybind11.h"
namespace py = pybind11;

namespace notf {

void decref_pyobject(PyObject* object)
{
    Py_XDECREF(object);
}

Foo::Foo()
    : py_object(nullptr, decref_pyobject)
{

}

Foo::~Foo()
{
    log_trace << "Removing Foo instance";
}

void Foo::set_pyobject(PyObject* object)
{
    assert(!py_object);
    Py_XINCREF(object);
    py_object.reset(std::move(object));
}

void Bar::do_foo() const
{
    log_info << "Bar foo";
}

std::vector<std::shared_ptr<Foo>> FooCollector::foos = {};

void add_foo(std::shared_ptr<Foo> foo)
{
    FooCollector::foos.push_back(foo);
}

void do_the_foos()
{
    FooCollector().do_the_foos();
}

}
