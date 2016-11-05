#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace notf {

// TODO: after the massacre, how do I integrate the ResourceManager?

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
     * @param texture_directory     System path to the texture directory, absolute or relative to the executable.
     */
    void set_texture_directory(std::string texture_directory);

    /**
     * @brief The Graphic Manager's shader directory path, absolute or relative to the executable.
     */
    const std::string& get_shader_directory() const { return m_shader_directory; }

    /**
     * @brief Sets a new shader directory.
     * @param shader_directory      System path to the shader directory, absolute or relative to the executable.
     */
    void set_shader_directory(std::string shader_directory);

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

};

} // namespace notf
