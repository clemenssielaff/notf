#pragma once

#include <memory>
#include <string>

struct NVGcontext;

namespace notf {

/** A text font.
 * All font handling is done by NanoVG or stb_truetype respectively, this class only acts as a convenience object to
 * access a particular font (and as jumping-off point, should we ever require better font-handling).
 */
class Font {

public: // static methods
    /** Loads a texture from a given file.
    * @param nvg_context        NanoVG context of which this Font is a part of.
    * @param name               Name of the Font, is also the filename of the *.ttf file.
    * @param font_path          Absolute path to a Font file.
    * @throw std::runtime_error If the Font fails to load.
    */
    static std::shared_ptr<Font> load(NVGcontext* nvg_context, const std::string& name, const std::string& font_path);

public: // methods
    /// no copy / assignment
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    /** Returns the ID of this Font. */
    int get_id() const { return m_id; }

protected: // methods
    Font(const int id)
        : m_id(id)
    {
    }

public: // static fields
    /** The file extension associated with Font files (.ttf). */
    static const std::string file_extension;

private: // fields
    /** ID of this Font. */
    const int m_id;
};

} // namespace notf
