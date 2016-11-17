#include "core/resource_manager.hpp"

#include <assert.h>

#include "common/hash_utils.hpp"
#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "graphics/font.hpp"
#include "graphics/texture2.hpp"

namespace { // anonymous

/** Helper function to clean up a map to shared pointers.
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

/** Makes sure that a given (directory) string ends in a forward slash for concatenation with a file name.
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

void ResourceManager::set_font_directory(std::string font_directory)
{
    m_font_directory = std::move(font_directory);
    if (!m_font_directory.empty()) {
        ensure_ends_in_forward_slash(m_font_directory);
    }
}

void ResourceManager::set_nvg_context(NVGcontext* context)
{
    assert(!m_context);
    m_context = context;
}

void ResourceManager::load_font(const std::string& name, const std::string& font_path)
{
    const std::string full_path = m_font_directory + font_path;
    std::shared_ptr<Font> font = Font::load(m_context, full_path, name);
    const size_t hash_value = hash(name);
    m_fonts.emplace(hash_value, font);
}

std::shared_ptr<Font> ResourceManager::get_font(const std::string& font_name)
{
    const size_t hash_value = hash(font_name);
    if (m_fonts.count(hash_value) == 0) {
        std::string message = string_format("Failed to retrieve unknown Font \"%s\"", font_name.c_str());
        log_critical << message;
        throw std::runtime_error(message);
    }
    return m_fonts.at(hash_value);
}

std::shared_ptr<Texture2> ResourceManager::get_texture(const std::string& texture_path, int flags)
{
    const size_t hash_value = hash(texture_path, flags);

    // return the existing texture, if it has already been loaded once
    if (m_textures.count(hash_value)) {
        return m_textures.at(hash_value);
    }

    // load the texture
    const std::string full_path = m_texture_directory + texture_path;
    std::shared_ptr<Texture2> texture = Texture2::load(m_context, full_path, flags);

    // store and return the texture
    m_textures.emplace(hash_value, texture);
    return texture;
}

void ResourceManager::cleanup()
{
    remove_unused(m_textures);
    // at the moment, fonts cannot be removed from NanoVG...
}

void ResourceManager::clear()
{
    m_textures.clear();
    m_fonts.clear();
}

} // namespace notf
