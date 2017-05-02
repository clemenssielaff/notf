#pragma once

#include <string>

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

private: // fields
    /** Used by Python to find the run-time libraries relative to the interpreter executable. */
    wchar_t* m_program;

    /** The application directory from which to parse the `main` module. */
    const std::string m_app_directory;
};

} // namespace notf
