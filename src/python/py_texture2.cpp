#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
#include "graphics/texture2.hpp"
using namespace notf;

void produce_texture2(pybind11::module& module)
{
    py::class_<Texture2, std::shared_ptr<Texture2>> Py_Texture2(module, "_Texture2");

    py::enum_<Texture2::Flags>(Py_Texture2, "Flags")
        .value("GENERATE_MIPMAPS", Texture2::Flags::GENERATE_MIPMAPS)
        .value("REPEATX", Texture2::Flags::REPEATX)
        .value("REPEATY", Texture2::Flags::REPEATY)
        .value("FLIPY", Texture2::Flags::FLIPY)
        .value("PREMULTIPLIED", Texture2::Flags::PREMULTIPLIED)
        .export_values();

    module.def("Texture2", [](const std::string& texture_path, int flags = Texture2::Flags::GENERATE_MIPMAPS) -> std::shared_ptr<Texture2> {
        return Application::get_instance().get_resource_manager().get_texture(texture_path, flags);
    }, "Retrieves a Texture2 by its path.", py::arg("texture_path"), py::arg("flags") = 1);
}
