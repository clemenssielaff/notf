#pragma once

#include <memory>
#include <string>
#include <unordered_map>

struct NVGcontext;

namespace notf {

class Font;
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

    /** The Graphic Manager's font directory path, absolute or relative to the executable. */
    const std::string& get_font_directory() const { return m_font_directory; }

    /** Sets a new font directory.
     * @param font_directory        System path to the font directory, absolute or relative to the executable.
     */
    void set_font_directory(std::string font_directory);

    /** Sets the NanoVG context into which to load the Textures. */
    void set_nvg_context(NVGcontext* context); // TODO: Ressource management won't work with multiple contexts.

    /** Loads a new Font from the given file and assigns it the given name.
     * @param name                  Name of the font.
     * @param font_path             Path of the font file, relative to the font directory.
     * @throw std::runtime_error    If the Font cannot be loaded.
     */
    void load_font(const std::string& name, const std::string& font_path);

    /** Retrieves a Font by its name.
     * Note that the Font must already been loaded using load_font.
     * @param font_name             Name of the Font.
    *  @throw std::runtime_error    If the requested Font is unknown.
     */
    std::shared_ptr<Font> get_font(const std::string& font_name);

    /** Retrieves a Texture2 by its path.
     * This function either loads the texture from disk if this is the first time it has been requested,
     * or reuses a cached texture, if it was already loaded.
     * @param texture_path          Texture path, relative to the texture directory.
     * @param flags                 Texture2::Flags used to load the texture.
    *  @throw std::runtime_error    If loading a Texture from the given path failed.
     */
    std::shared_ptr<Texture2> get_texture(const std::string& texture_path, int flags);

    /** Deletes all resources that are not currently being used. */
    void cleanup();

    /** Releases ownership of all managed resources.
     * If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
     */
    void clear();

private: // fields
    /** System path to the texture directory, absolute or relative to the executable. */
    std::string m_texture_directory;

    /** System path to the font directory, absolute or relative to the executable. */
    std::string m_font_directory;

    /** All managed Textures - indexed by hash of name relative to the texture directory and texture flags. */
    std::unordered_map<size_t, std::shared_ptr<Texture2>> m_textures;

    /** All managed Fonts - indexed by hash of name relative to the font directory. */
    std::unordered_map<size_t, std::shared_ptr<Font>> m_fonts;

    /** NanoVG context in which the textures live. */
    NVGcontext* m_context;
};

} // namespace notf
