#pragma once

#include <vector>
#include <string>

#include "pybind11/pybind11.h"
namespace py = pybind11;

namespace notf {

class PythonInterpreter {

public: // methods
    /**
     * @param argv              Command line arguments passed to the Application.
     * @param app_directory     The application directory from which to parse the `main` module.
     */
    PythonInterpreter(char* argv[], const std::string& app_directory);

    ~PythonInterpreter();

    /** (Re-)Parses the user app, completely clears out the global and local namespace.
     * @param filename      Name of the app's `main` module, located in the app directory.
     */
    void parse_app(const std::string& filename);

private: // methods
    /** Produces a fresh `globals` dictionary for each run of `parse_app`.
     * @param filename      Absolute path to the file, is put into `__file__`.
     */
    py::object _build_globals(const std::string& filename) const;

private: // fields
    /** Used by Python to find the run-time libraries relative to the interpreter executable. */
    wchar_t* m_program;

    /** The application directory from which to parse the `main` module. */
    const std::string m_app_directory;

    py::set m_object_cache;
};

} // namespace notf
