#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
using namespace notf;

namespace { // anonymous

struct ResourceManagerWrapper {

    void load_font(const std::string& name, const std::string& font_path)
    {
        Application::get_instance().get_resource_manager().load_font(name, font_path);
    }
};

} // namespace anonymous

void produce_resource_manager(pybind11::module& module)
{
    py::class_<ResourceManagerWrapper> Py_ResourceManager(module, "ResourceManager");

    Py_ResourceManager.def("load_font", &ResourceManagerWrapper::load_font, "Loads a new Font from the given file and assigns it the given name.", py::arg("name"), py::arg("path"));
}
