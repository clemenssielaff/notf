#include "core/resource_manager.hpp"

#include "common/log.hpp"
#include "common/string_utils.hpp"

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

void ResourceManager::cleanup()
{
}

void ResourceManager::clear()
{
}

} // namespace notf
