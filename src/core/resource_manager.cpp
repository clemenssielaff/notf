#include "core/resource_manager.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "graphics/texture2.hpp"

namespace { // anonymous

/**
 * @brief Helper function to clean up a map to shared pointers.
 *
 * Cleaning up in this case means removing all entries where the shared pointer is the only one managing its data.
 */
template <typename Type>
void remove_unused(Type& member)
{
    Type member_swap;
    for (auto it = member.cbegin(); it != member.cend(); ++it) {
        if (!it->second.unique()) {
            member_swap.emplace(std::move(*it));
        }
    }
    std::swap(member, member_swap);
}

/**
 * @brief Makes sure that a given (directory) string ends in a forward slash for concatenation with a file name.
 * @param input Input string.
 */
void ensure_ends_in_forward_slash(std::string& input)
{
    if (input.back() != '/') {
        input.push_back('/');
    }
}

} // namespace anonymous

namespace notf {

void ResourceManager::set_texture_directory(std::string texture_directory)
{
    m_texture_directory = std::move(texture_directory);
    if (!m_texture_directory.empty()) {
        ensure_ends_in_forward_slash(m_texture_directory);
    }
}

void ResourceManager::set_shader_directory(std::string shader_directory)
{
    m_shader_directory = std::move(shader_directory);
    if (!m_shader_directory.empty()) {
        ensure_ends_in_forward_slash(m_shader_directory);
    }
}

void ResourceManager::set_nvg_context(NVGcontext* context)
{
    assert(!m_context);
    m_context = context;
}

// TODO: load a new texture if the flags differ
std::shared_ptr<Texture2> ResourceManager::get_texture(const std::string& texture_path, int flags)
{
    // return the existing texture, if it has already been loaded once
    if (m_textures.count(texture_path)) {
        return m_textures.at(texture_path);
    }

    // load the texture
    const std::string full_path = m_texture_directory + texture_path;
    std::shared_ptr<Texture2> texture = Texture2::load(m_context, full_path, flags);

    // store and return the texture
    m_textures.emplace(texture_path, texture);
    return texture;
}

void ResourceManager::cleanup()
{
    remove_unused(m_textures);
}

void ResourceManager::clear()
{
    m_textures.clear();
}

} // namespace notf
