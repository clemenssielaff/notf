#pragma once

#include <unordered_map>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/pointer.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

NOTF_EXCEPTION_TYPE(resource_manager_initialization_error);

NOTF_EXCEPTION_TYPE(resource_identification_error);

// ================================================================================================================== //

/// The Resource Manager owns all dynamically loaded resources.
/// It is not a Singleton, even though each Application will most likely only have one.
/// The ResourceManager does not load individual resources, it only stores them and allows other parts of the
/// application to get them by name.
class ResourceManager : public receive_signals {

    // types -------------------------------------------------------------------------------------------------------- //
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

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(ResourceManager);

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

    /// Deletes all resources that are not currently being used.
    void cleanup();

    /// Releases ownership of all managed resources.
    /// If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
    void clear();

    // getter -----------------------------------------------------------------

    /// Returns a Shader by name
    /// @param name Name of the Shader.
    /// @returns The requested Shader, may be empty if there is no shader with the given name.
    risky_ptr<ShaderPtr> shader(const std::string& name);

    /// Returns a Texture by name
    /// @param name Name of the Texture.
    /// @returns The requested Texture, may be empty if there is no shader with the given name.
    risky_ptr<TexturePtr> texture(const std::string& name);

    /// Returns a Font by name
    /// @param name Name of the Font.
    /// @returns The requested Font, may be empty if there is no shader with the given name.
    risky_ptr<FontPtr> font(const std::string& name);

private:
    /// Adds a new Shader, does nothing if the Shader is already stored in the manager.
    /// @param shader   Shader to add.
    /// @throws resource_identification_error   If the Shader has the same name as an existing Shader.
    void _add_shader(ShaderPtr shader);

    /// Adds a new Texture, does nothing if the Texture is already stored in the manager.
    /// @param texture  Texture to add.
    /// @throws resource_identification_error   If the Texture has the same name as an existing Texture.
    void _add_texture(TexturePtr texture);

    /// Adds a new Font, does nothing if the Font is already stored in the manager.
    /// @param font     Font to add.
    /// @throws resource_identification_error   If the Font has the same name as an existing Font.
    void _add_font(FontPtr font);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Absolute path to the texture directory.
    std::string m_texture_directory;

    /// Absolute path to the shader directory.
    std::string m_shader_directory;

    /// System path to the font directory, absolute or relative to the executable.
    std::string m_font_directory;

    /// All shaders stored in the manager.
    std::unordered_map<std::string, ShaderPtr> m_shaders;

    /// All Textures stored in the manager.
    std::unordered_map<std::string, TexturePtr> m_textures;

    /// All Fonts stored in the manager.
    std::unordered_map<std::string, FontPtr> m_fonts;
};

NOTF_CLOSE_NAMESPACE
