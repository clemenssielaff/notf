#include "python/interpreter.hpp"

#include <cstdio>
#include <iostream>

#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;
using namespace py::literals;

#include "common/log.hpp"
#include "common/string.hpp"
#include "core/application.hpp"
#include "python/py_notf.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

PythonInterpreter::PythonInterpreter(char* argv[], const std::string& app_directory)
    : m_program(Py_DecodeLocale(argv[0], nullptr))
    , m_app_directory(app_directory)
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

    const std::string path_command = string_format(
        "import sys;"
        "sys.path.append(\"\"\"%s\"\"\");"
        "sys._original_modules = sys.modules.copy();",
        app_directory.c_str());
    PyRun_SimpleString(path_command.c_str());
}

PythonInterpreter::~PythonInterpreter()
{
    Py_Finalize();
    PyMem_RawFree(m_program);
    log_trace << "Closed Python interpreter";
}

void PythonInterpreter::parse_app(const std::string& filename)
{
    log_format << "";
    log_format << "================================================================================";

    // find and open the file to execute
    std::string absolute_path = m_app_directory + filename;
    FILE* fp = std::fopen(absolute_path.c_str(), "r");
    if (!fp) {
        log_warning << "Could not parse empty source code read from " << absolute_path;
        log_format << "================================================================================";
        return;
    }

    log_format << " Parsing app code from: " << absolute_path;
    log_format << "================================================================================";

    // build the globals dict
    py::object globals = py::dict(
        "__builtins__"_a = py::handle(PyEval_GetBuiltins()),
        "__name__"_a = py::str("__main__"),
        "__package__"_a = py::none(),
        "__doc__"_a = py::none(),
        "__spec__"_a = py::none(),
        "__loader__"_a = py::none(),
        "__file__"_a = py::str(filename.c_str()));
    if (!globals.check()) {
        log_critical << "Failed to build the 'globals' dictionary!";
        std::fclose(fp);
        return;
    }

    // resture the original modules (force-reload all custom ones)
    static const std::string restore_original_modules_command = "modules_to_remove = []\n"
                                                                "for module in sys.modules.keys():\n"
                                                                "    if not module in sys._original_modules:\n"
                                                                "        modules_to_remove.append(module)\n"
                                                                "for module in modules_to_remove:\n"
                                                                "    del(sys.modules[module])\n";
    PyRun_SimpleString(restore_original_modules_command.c_str());

    // run the script
    py::object _(PyRun_FileEx(fp, absolute_path.c_str(), Py_file_input, globals.ptr(), globals.ptr(),
                              1), // close the file stream before returning
                 false); // the return object is a new reference

    // check for errors
    if (PyErr_Occurred()) {
        log_critical << "Python error occured: ";
        PyErr_Print();
    }
}

} // namespace notf
