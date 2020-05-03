#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "core/application.hpp"
#include "core/resource_manager.hpp"
#include "graphics/texture2.hpp"
#include "ext/python/docstr.hpp"
using namespace notf;

void produce_texture2(pybind11::module& module)
{
    py::class_<Texture2, std::shared_ptr<Texture2>> Py_Texture2(module, "Texture2");

    py::enum_<TextureFlags>(Py_Texture2, "TextureFlags")
        .value("GENERATE_MIPMAPS", TextureFlags::GENERATE_MIPMAPS)
        .value("REPEATX", TextureFlags::REPEATX)
        .value("REPEATY", TextureFlags::REPEATY)
        .value("FLIPY", TextureFlags::FLIPY)
        .value("PREMULTIPLIED", TextureFlags::PREMULTIPLIED)
        .export_values();

    Py_Texture2.def_static("fetch", [](const std::string& texture_path, int flags = TextureFlags::GENERATE_MIPMAPS) -> std::shared_ptr<Texture2> {
        return Application::get_instance().get_resource_manager().fetch_texture(texture_path, flags);
    }, DOCSTR("Retrieves a Texture2 by its path."), py::arg("texture_path"), py::arg("flags") = 1);
}
