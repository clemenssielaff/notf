#pragma once

#include <memory>
#include <string>
#include <unordered_map>

struct NVGcontext;

namespace notf {

class Font;
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

    /** Retrieves a Font by its name in the font directory
     * There is a 1:1 relationship between a font name and its .ttf file.
     * This is also the reason why you can only load fonts from the font directory as we can guarantee that there will
     * be no naming conflicts.
     * @param font_name             Name of the Font and its file in the font directory (the *.ttf ending is optional).
     * @throw std::runtime_error    If the requested Font cannot be loaded.
     */
    std::shared_ptr<Font> fetch_font(const std::string& name);

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

    /** All managed Fonts - indexed by hash of name relative to the font directory. */
    std::unordered_map<size_t, std::shared_ptr<Font>> m_fonts;

    /** NanoVG context in which the textures live. */
    NVGcontext* m_context;
};

} // namespace notf
