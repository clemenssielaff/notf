#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
#include "graphics/font.hpp"
#include "python/docstr.hpp"
using namespace notf;

void produce_font(pybind11::module& module)
{
    py::class_<Font, std::shared_ptr<Font>> Py_Font(module, "_Font");

    Py_Font.def_static("fetch", [](const std::string& name) -> std::shared_ptr<Font> {
        return Application::get_instance().get_resource_manager().fetch_font(name);
    }, DOCSTR("Name of the Font and its file in the font directory (the *.ttf ending is optional)."), py::arg("name"));
}
