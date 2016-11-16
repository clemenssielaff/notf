#pragma once

#include <memory>
#include <string>
#include <unordered_map>

struct NVGcontext;

namespace notf {

class Texture2;

/** The Resource Manager owns all dynamically loaded resources.
 * It is not a Singleton, even though each Application will most likely only have one.
 */
class ResourceManager {

public: // methods
    ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete; // no copy construction
    ResourceManager& operator=(const ResourceManager&) = delete; // no copy assignment

    /** The Graphic Manager's texture directory path, absolute or relative to the executable. */
    const std::string& get_texture_directory() const { return m_texture_directory; }

    /** Sets a new texture directory.
     * @param texture_directory     System path to the texture directory, absolute or relative to the executable.
     */
    void set_texture_directory(std::string texture_directory);

    /** The Graphic Manager's shader directory path, absolute or relative to the executable. */
    const std::string& get_shader_directory() const { return m_shader_directory; }

    /** Sets a new shader directory.
     * @param shader_directory      System path to the shader directory, absolute or relative to the executable.
     */
    void set_shader_directory(std::string shader_directory);

    /** Sets the NanoVG context into which to load the Textures. */
    void set_nvg_context(NVGcontext* context); // TODO: Ressource management won't work with multiple contexts.

    /** Retrieves a Texture2 by its path.
     * This function either loads the texture from disk if this is the first time it has been requested,
     * or reuses a cached texture, if it was already loaded.
     * @param texture_path          Texture path, relative to the texture directory.
     * @param flags                 Texture2::Flags used to load the texture.
    *  @throw std::runtime_error    If loading a Texture from the given path failed.
     */
    std::shared_ptr<Texture2> get_texture(const std::string& texture_path, int flags = 0);

    /** Deletes all resources that are not currently being used. */
    void cleanup();

    /** Releases ownership of all managed resources.
     * If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
     */
    void clear();

private: // fields
    /** System path to the texture directory, absolute or relative to the executable. */
    std::string m_texture_directory;

    /** System path to the shader directory, absolute or relative to the executable. */
    std::string m_shader_directory;

    /** All managed Textures - indexed by name relative to the texture directory. */
    std::unordered_map<std::string, std::shared_ptr<Texture2>> m_textures;

    /** NanoVG context in which the textures live. */
    NVGcontext* m_context;
};

} // namespace notf
