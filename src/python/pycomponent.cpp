#include "python/pywidget.hpp"

#include "core/component.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

py::class_<Component, std::shared_ptr<Component>> produce_component(pybind11::module& module)
{
    py::class_<Component, std::shared_ptr<Component>> Py_Component(module, "_Component");

    return Py_Component;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
