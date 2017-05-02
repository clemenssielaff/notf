#include "ext/python/py_fwd.hpp"

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

char* py_print(PyObject* object)
{
    return PyUnicode_AsUTF8(PyObject_Repr(object));
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace notf
