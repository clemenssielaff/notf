#include "app/core/resource_manager.hpp"

namespace { // anonymous

/// Helper function to clean up a map to shared pointers.
/// Cleaning up in this case means removing all entries where the shared pointer is the only one managing its data.
template<typename Type>
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

} // namespace

//====================================================================================================================//

namespace notf {

resource_manager_initialization_error::~resource_manager_initialization_error() {}

//====================================================================================================================//

ResourceManager::ResourceManager(Args&& args) : m_texture_directory(), m_shader_directory(), m_font_directory()
{
    const std::string executable_directory = args.executable_path.substr(0, args.executable_path.find_last_of("/") + 1);

    m_texture_directory = make_absolute(args.texture_directory, executable_directory);
    m_shader_directory  = make_absolute(args.shader_directory, executable_directory);
    m_font_directory    = make_absolute(args.fonts_directory, executable_directory);
}

void ResourceManager::cleanup() {}

void ResourceManager::clear() {}

} // namespace notf
