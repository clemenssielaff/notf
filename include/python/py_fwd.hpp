#pragma once

struct _object;
using PyObject = _object;

struct _typeobject;
using PyTypeObject = _typeobject;

namespace notf {

/** Py_XINCREF wrapper. */
void py_incref(PyObject* object);

/** Py_XDECREF wrapper. */
void py_decref(PyObject* object);

/** Returns the print() representation of the given Python object. */
char* py_print(PyObject* object);

} // namespace notf
