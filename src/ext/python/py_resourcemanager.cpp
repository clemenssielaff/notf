#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

namespace { // anonymous

struct ResourceManagerWrapper {
    void cleanup() { Application::get_instance().get_resource_manager().cleanup(); }
};

} // namespace anonymous

void produce_resource_manager(pybind11::module& module)
{
    py::class_<ResourceManagerWrapper> Py_ResourceManager(module, "ResourceManager");

    Py_ResourceManager.def(py::init<>());
    Py_ResourceManager.def("cleanup", &ResourceManagerWrapper::cleanup, DOCSTR("Deletes all resources that are not currently being used."));
}
