#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/component.hpp"
using namespace notf;

py::class_<Component, std::shared_ptr<Component>> produce_component(pybind11::module& module)
{
    return py::class_<Component, std::shared_ptr<Component>>(module, "_Component");
}
