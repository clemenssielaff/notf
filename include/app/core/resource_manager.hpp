#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "graphics/forwards.hpp"

namespace notf {

//====================================================================================================================//

NOTF_EXCEPTION_TYPE(resource_manager_initialization_error);

//====================================================================================================================//

/// The Resource Manager owns all dynamically loaded resources.
/// It is not a Singleton, even though each Application will most likely only have one.
class ResourceManager {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// ResourceManager arguments.
    struct Args {
        /// System path to the texture directory, can either be absolute or relative to the executable.
        std::string texture_directory;

        /// System path to the fonts directory, absolute or relative to the executable.
        std::string fonts_directory;

        /// System path to the shader directory, absolute or relative to the executable.
        std::string shader_directory;

        /// Absolute path to this executable (is used to resolve the other paths, if they are relative).
        std::string executable_path;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NO_COPY_AND_ASSIGN(ResourceManager)

    /// Construction.
    /// @param args Arguments.
    /// @throws resource_manager_initialization_error
    ResourceManager(Args&& args);

    /// The absolute texture directory path.
    const std::string& texture_directory() const { return m_texture_directory; }

    /// The absolute shader directory path
    const std::string& shader_directory() const { return m_shader_directory; }

    /// The absolute font directory path
    const std::string& font_directory() const { return m_font_directory; }

    //    /// Retrieves a Font by its name in the font directory
    //    /// There is a 1:1 relationship between a font name and its .ttf file.
    //    /// This is also the reason why you can only load fonts from the font directory as we can guarantee that there
    //    will
    //    /// be no naming conflicts.
    //    /// @param font_name             Name of the Font and its file in the font directory (the *.ttf ending is
    //    optional).
    //    /// @throw std::runtime_error    If the requested Font cannot be loaded.
    //    FontPtr fetch_font(const std::string& name);

    /// Deletes all resources that are not currently being used.
    void cleanup();

    /// Releases ownership of all managed resources.
    /// If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
    void clear();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Absolute path to the texture directory.
    std::string m_texture_directory;

    /// Absolute path to the shader directory.
    std::string m_shader_directory;

    /// System path to the font directory, absolute or relative to the executable.
    std::string m_font_directory;

    /// All managed Fonts - indexed by hash of name relative to the font directory.
    //    std::unordered_map<size_t, FontPtr> m_fonts;
};

} // namespace notf
