#include "core/resource_manager.hpp"

#include <assert.h>

#include "common/hash.hpp"
#include "common/log.hpp"
#include "common/string.hpp"
#include "common/warnings.hpp"

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

std::shared_ptr<Font> ResourceManager::fetch_font(const std::string& name)
{
    UNUSED(name);
    return {};
    //    std::string full_path = m_font_directory + name;
    //    if(!iends_with(name, Font::file_extension)){
    //        full_path.append(Font::file_extension);
    //    }

    //    const size_t hash_value = hash(full_path);
    //    if (m_fonts.count(hash_value)) {
    //        return m_fonts.at(hash_value);
    //    }

    //    std::shared_ptr<Font> font = Font::load(m_context, name, full_path);
    //    m_fonts.emplace(hash_value, font);
    //    return font;
}

void ResourceManager::cleanup()
{
}

void ResourceManager::clear()
{
    m_fonts.clear();
}

} // namespace notf
