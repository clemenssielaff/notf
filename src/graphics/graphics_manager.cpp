#include "graphics/graphics_manager.hpp"

#include "common/log.hpp"

namespace { // anonymous

/**
 * \brief Helper function to clean up a map to shared pointers.
 *
 * Cleaning up in this case means removing all entries where the shared pointer is the only one managing its data.
 */
template <typename Type>
void remove_uniques(Type& member)
{
    Type member_swap;
    for( auto it = member.cbegin(); it != member.cend(); ++it){
        if(!it->second.unique()){
            member_swap.emplace(std::move(*it));
        }
    }
    std::swap(member, member_swap);
}

} // namespace anonymous

namespace signal {

GraphicsManager::GraphicsManager(std::string texture_directory, std::string shader_directory)
    : m_texture_directory(std::move(texture_directory))
    , m_shader_directory(std::move(shader_directory))
    , m_textures()
    , m_shaders()
{
}

std::shared_ptr<Texture2> GraphicsManager::get_texture(const std::string& texture_path)
{
    // return the existing texture, if it has already been loaded once
    if (m_textures.count(texture_path)) {
        return m_textures.at(texture_path);
    }

    // load the texture
    const std::string full_path = m_texture_directory + texture_path;
    std::shared_ptr<Texture2> texture = Texture2::load(full_path);
    if (!texture) {
        return {};
    }

    // store and return the texture
    m_textures.emplace(texture_path, texture);
    return texture;
}

std::shared_ptr<Shader> GraphicsManager::get_shader(const std::string& shader_name)
{
    if (m_shaders.count(shader_name)) {
        return m_shaders.at(shader_name);
    }
    else {
        return {};
    }
}

std::shared_ptr<Shader> GraphicsManager::build_shader(const std::string& shader_name,
                                                      const std::string& vertex_shader_path,
                                                      const std::string& fragment_shader_path,
                                                      const std::string& geometry_shader_path)
{
    // if the name already identifies a shader, return that one instead
    if (m_shaders.count(shader_name)) {
        return m_shaders.at(shader_name);
    }

    // load the shader
    const std::string full_vertex_path = m_shader_directory + vertex_shader_path;
    const std::string full_fragment_path = m_shader_directory + fragment_shader_path;
    const std::string full_geometry_path = geometry_shader_path.empty() ? "" : m_shader_directory + geometry_shader_path;
    std::shared_ptr<Shader> shader = Shader::build(full_vertex_path, full_fragment_path, full_geometry_path);
    if (!shader) {
        return {};
    }

    // store and return the texture
    log_info << "Compiled shader '" << shader_name << "'";
    m_shaders.emplace(shader_name, shader);
    return shader;
}

void GraphicsManager::cleanup()
{
    remove_uniques(m_textures);
    remove_uniques(m_shaders);
}

} // namespace signal
