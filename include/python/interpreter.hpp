#pragma once

#include <string>

namespace pybind11 {
class object;
}
namespace py = pybind11;

namespace notf {

class PythonInterpreter {

public: // methods
    /// @param argv     Command line arguments passed to the Application.
    PythonInterpreter(char* argv[]);

    ~PythonInterpreter();

    /// @brief (Re-)Parses the user app, completely clears out the global and local namespace.
    /// @param filename     Name of the app's `main` module.
    void parse_app(const std::string& filename = "/home/clemens/code/notf/app/main.py");

private: // methods
    py::object build_globals() const;

private: // fields
    /// @brief Used by Pythonto find the run-time libraries relative to the interpreter executable.
    wchar_t* m_program;
};

} // namespace notf
