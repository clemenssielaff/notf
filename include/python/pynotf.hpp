#pragma once

struct _object;
typedef _object PyObject;

static const char* python_notf_module_name = "notf";

/**
 * @brief Produces the NoTF python module that is available in the embedded Python interpreter.
 */
PyObject* produce_pynotf_module();
