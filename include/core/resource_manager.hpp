#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace notf {

class Shader;
class Texture2;

/**
 * @brief The Resource Manager owns all dynamically loaded resources.
 *
 * It is not a Singleton, even though each Application will most likely only have one.
 */
class ResourceManager {

public: // methods
    /**
     * @brief Default Constructor.
     */
    explicit ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete; // no copy construction
    ResourceManager& operator=(const ResourceManager&) = delete; // no copy assignment

    /**
     * @brief The Graphic Manager's texture directory path, absolute or relative to the executable.
     */
    const std::string& get_texture_directory() const { return m_texture_directory; }

    /**
     * @brief Sets a new texture directory.
     * @param texture_directory System path to the texture directory, absolute or relative to the executable.
     */
    void set_texture_directory(std::string texture_directory);

    /**
     * @brief The Graphic Manager's shader directory path, absolute or relative to the executable.
     */
    const std::string& get_shader_directory() const { return m_shader_directory; }

    /**
     * @brief Sets a new shader directory.
     * @param shader_directory  System path to the shader directory, absolute or relative to the executable.
     */
    void set_shader_directory(std::string shader_directory);

    /**
     * @brief Retrieves a texture by its path.
     *
     * This function either loads the texture from disk if this is the first time it has been requested,
     * or reuses a cached texture, if it was already loaded.
     *
     * @param texture_path  Texture path, relative to the texture directory.
     * @return The loaded Texture - or an empty pointer, if an error occurred.
     */
    std::shared_ptr<Texture2> get_texture(const std::string& texture_path);

    /**
     * @brief Retrieves a texture by its name.
     *
     * The Shader must have been built before retrieval using `build_shader`;
     *
     * @param shader_name   Name of the texture to retrieve.
     * @return The Shader - or an empty pointer, if the name does not identify a Shader.
     */
    std::shared_ptr<Shader> get_shader(const std::string& shader_name);

    /**
     * @brief Loads several shader source files from disk and compiles an OpenGL shader from them.
     *
     * If the given name already identifies a Shader, it is returned instead.
     *
     * @param shader_name           User-defined name for the Shader.
     * @param vertex_shader_path    Path to a vertex shader source file.
     * @param fragment_shader_path  Path to a fragment shader source file.
     * @param geometry_shader_path  (optional) Path to a geometry shader source file.
     * @return The requested Shader - or an empty pointer, if an error occurred.
     */
    std::shared_ptr<Shader> build_shader(const std::string& shader_name,
                                         const std::string& vertex_shader_path,
                                         const std::string& fragment_shader_path,
                                         const std::string& geometry_shader_path = "");

    /**
     * @brief Deletes all resources that are not currently being used.
     */
    void cleanup();

    /**
     * @brief Releases ownership of all managed resources
     *
     * If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
     */
    void clear();

private: // fields
    /**
     * @brief System path to the texture directory, absolute or relative to the executable.
     */
    std::string m_texture_directory;

    /**
     * @brief System path to the shader directory, absolute or relative to the executable.
     */
    std::string m_shader_directory;

    /**
     * @brief All managed Textures - indexed by name relative to the texture directory.
     */
    std::unordered_map<std::string, std::shared_ptr<Texture2>> m_textures;

    /**
     * @brief All managed Shaders - indexed by a user-assigned name.
     */
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
};

} // namespace notf
