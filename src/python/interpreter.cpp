#include "python/interpreter.hpp"

#include <cstdio>
#include <iostream>

#include "Python.h"

#include "common/log.hpp"
#include "core/application.hpp"
#include "python/pynotf.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

PythonInterpreter::PythonInterpreter(char* argv[])
    : m_program(Py_DecodeLocale(argv[0], 0))
{
    // prepare the interpreter
    if (!m_program) {
        log_fatal << "Fatal Python error: cannot decode argv[0]";
        exit(to_number(Application::RETURN_CODE::PYTHON_FAILURE));
    }
    Py_SetProgramName(m_program);

    // make sure that the notf module is available alongside the built-in Python modules
    PyImport_AppendInittab(python_notf_module_name, &produce_pynotf_module);

    Py_Initialize();
    log_info << "Initialized Python interpreter " << Py_GetVersion();
}

PythonInterpreter::~PythonInterpreter()
{
    Py_Finalize();
    PyMem_RawFree(m_program);
    log_trace << "Closed Python interpreter";
}

void PythonInterpreter::parse_app(const std::string& filename)
{
    FILE* fp = std::fopen(filename.c_str(), "r");
    if (!fp) {
        log_warning << "Could not parse empty source code read from " << filename;
        return;
    }

    PyObject* globals = build_globals();
    if (!globals) {
        log_critical << "Failed to build the globals dictionary!";
        return;
    }

    PyObject* result = PyRun_FileEx(fp, filename.c_str(), Py_file_input, globals, globals, 1);
    if (PyErr_Occurred()) {
        log_critical << "Python error occured: ";
        PyErr_Print();
    }

    Py_DecRef(result);
    Py_DecRef(globals);

    log_trace << "Reparsed app code from: " << filename;
}

PyObject* PythonInterpreter::build_globals() const
{
    PyObject* globals = PyDict_New();
    int result = PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
    result += PyDict_SetItemString(globals, "__name__", PyUnicode_FromString("__main__"));
    result += PyDict_SetItemString(globals, "__package__", Py_None);
    result += PyDict_SetItemString(globals, "__doc__", Py_None);
    result += PyDict_SetItemString(globals, "__spec__", Py_None);
    result += PyDict_SetItemString(globals, "__loader__", Py_None);
    if (result != 0) {
        Py_DecRef(globals);
        globals = nullptr;
    }
    return globals;
}

} // namespace notf
