#include "python/pyobject_wrapper.hpp"

#include <Python.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

namespace notf {

void py_incref(PyObject* object)
{
    Py_XINCREF(object);
}

void py_decref(PyObject* object)
{
    Py_XDECREF(object);
//    --object->ob_type->ob_base.ob_base.ob_refcnt;
}

} // namespace notf

#ifdef __clang__
#pragma clang diagnostic pop
#endif
