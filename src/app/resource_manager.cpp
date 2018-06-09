#include "app/resource_manager.hpp"

#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/text/font.hpp"

namespace { // anonymous

/// Makes sure that a given (directory) string ends in a forward slash for concatenation with a file name.
/// @param path         Absolute or relative path.
/// @param root_dir     If the path is relative, then it will be made absolute in relation to this path.
std::string make_absolute(const std::string& path, const std::string& root_dir)
{
    std::string result;
    if (path.empty() || path[0] != '/') {
        result = root_dir + path;
    }
    else {
        result = path;
    }

    if (result.back() != '/') {
        result.push_back('/');
    }

    return result;
}

/// Helper function to clean up a map to shared pointers.
/// Cleaning up in this case means removing all entries where the shared pointer is the only one managing its data.
template<typename T>
void remove_unused(T& container)
{
    T swap_container;
    for (auto it = container.cbegin(); it != container.cend(); ++it) {
        if (!it->second.unique()) {
            swap_container.emplace(std::move(*it));
        }
    }
    std::swap(container, swap_container);
}

} // namespace

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

resource_manager_initialization_error::~resource_manager_initialization_error() {}

resource_identification_error::~resource_identification_error() {}

// ================================================================================================================== //

ResourceManager::ResourceManager(Args&& args)
{
    const std::string executable_directory = args.executable_path.substr(0, args.executable_path.find_last_of('/') + 1);
    m_texture_directory = make_absolute(args.texture_directory, executable_directory);
    m_shader_directory = make_absolute(args.shader_directory, executable_directory);
    m_font_directory = make_absolute(args.fonts_directory, executable_directory);

    // connect signals
    connect_signal(VertexShader::on_shader_created, &ResourceManager::_add_shader);
    connect_signal(TesselationShader::on_shader_created, &ResourceManager::_add_shader);
    connect_signal(GeometryShader::on_shader_created, &ResourceManager::_add_shader);
    connect_signal(FragmentShader::on_shader_created, &ResourceManager::_add_shader);
    connect_signal(Texture::on_texture_created, &ResourceManager::_add_texture);
    connect_signal(Font::on_font_created, &ResourceManager::_add_font);
}

void ResourceManager::cleanup()
{
    remove_unused(m_shaders);
    remove_unused(m_textures);
    remove_unused(m_fonts);
}

void ResourceManager::clear()
{
    m_shaders.clear();
    m_textures.clear();
    m_fonts.clear();
}

risky_ptr<ShaderPtr> ResourceManager::shader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    return nullptr;
}

risky_ptr<TexturePtr> ResourceManager::texture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    return nullptr;
}

risky_ptr<FontPtr> ResourceManager::font(const std::string& name)
{
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) {
        return it->second;
    }
    return nullptr;
}

void ResourceManager::_add_shader(ShaderPtr shader)
{
    const std::string& name = shader->name();
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        m_shaders.insert(std::make_pair(name, shader)); // store a new shader
    }
    else {
        if (it->second == shader) {
            return; // do nothing if the same shader is already known
        }
        notf_throw(resource_identification_error,
                          "Cannot store Shader \"{}\" because another Shader with the same name is already in storage",
                          name)
    }
}

void ResourceManager::_add_texture(TexturePtr texture)
{
    const std::string& name = texture->name();
    auto it = m_textures.find(name);
    if (it == m_textures.end()) {
        m_textures.insert(std::make_pair(name, texture)); // store a new texture
    }
    else {
        if (it->second == texture) {
            return; // do nothing if the same texture is already known
        }
        notf_throw(
            resource_identification_error,
            "Cannot store Texture \"{}\" because another Texture with the same name is already in storage", name)
    }
}

void ResourceManager::_add_font(FontPtr font)
{
    const std::string& name = font->name();
    auto it = m_fonts.find(name);
    if (it == m_fonts.end()) {
        m_fonts.insert(std::make_pair(name, font)); // store a new font
    }
    else {
        if (it->second == font) {
            return; // do nothing if the same font is already known
        }
        notf_throw(resource_identification_error,
                          "Cannot store Font \"{}\" because another Font with the same name is already in storage",
                          name)
    }
}

NOTF_CLOSE_NAMESPACE
