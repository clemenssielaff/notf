#include "python/interpreter.hpp"

#include <cstdio>
#include <iostream>

#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;
using namespace pybind11::literals;

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

    py::object globals = build_globals();
    if (!globals.check()) {
        log_critical << "Failed to build the globals dictionary!";
        std::fclose(fp);
        return;
    }

    py::object _(PyRun_FileEx(fp, filename.c_str(), Py_file_input, globals.ptr(), globals.ptr(),
                              1), // close the file stream before returning
                 false); // the return object is a new reference


    if (PyErr_Occurred()) {
        log_critical << "Python error occured: ";
        PyErr_Print();
    }

    log_trace << "Reparsed app code from: " << filename;
}

py::object PythonInterpreter::build_globals() const
{
    py::dict globals = py::dict(
        "__builtins__"_a = py::handle(PyEval_GetBuiltins()),
        "__name__"_a = py::str("__main__"),
        "__package__"_a = py::none(),
        "__doc__"_a = py::none(),
        "__spec__"_a = py::none(),
        "__loader__"_a = py::none());
    return globals;
}

} // namespace notf
