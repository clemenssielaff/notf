#include "python/pyobject_wrapper.hpp"

#include <Python.h>

namespace notf {

void py_incref(PyObject* object)
{
    Py_XINCREF(object);
}

void py_decref(PyObject* object)
{
    Py_XDECREF(object);
}

} // namespace notf
