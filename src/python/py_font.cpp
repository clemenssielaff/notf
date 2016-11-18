#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
#include "graphics/font.hpp"
using namespace notf;

void produce_font(pybind11::module& module)
{
    py::class_<Font, std::shared_ptr<Font>>(module, "_Font");

    module.def("Font", [](const std::string& name) -> std::shared_ptr<Font> {
        return Application::get_instance().get_resource_manager().get_font(name);
    }, "Retrieves a Font by its name in the font directory.", py::arg("Name of the Font and its file in the font directory (the *.ttf ending is optional)."));
}
