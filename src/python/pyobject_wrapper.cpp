#include "python/pyobject_wrapper.hpp"

#include <Python.h>

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void py_incref(PyObject* object)
{
    Py_XINCREF(object);
}

void py_decref(PyObject* object)
{
    Py_XDECREF(object);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace notf
