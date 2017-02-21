#pragma once

#include <string>
#include <vector>

#include "thirdparty/stb_truetype/stb_truetype.h"

#include "common/aabr.hpp"
#include "common/size2i.hpp"

namespace notf {

struct FontInfo {
    const std::string name;
    const std::string filepath;
    Size2i atlas_size;

    FontInfo(std::string name, std::string filepath)
        : name(std::move(name))
        , filepath(std::move(filepath))
        , atlas_size{1024, 1024}
    {
    }
};

class Font {

    struct Glyph {
        uint codepoint;
        size_t index;
        size_t next;
        short size;
        short blur;
        Aabr rect;
        short x_advance;
        short x_offset;
        short y_offset;
    };

public: // static
    static Font load(FontInfo info);

private: // method
    /** Constructor.
     * Produces an invalid Font by itself, use Font::load to create a valid Font.
     */
    Font(FontInfo info);

public: // methods
    ~Font();

    const std::string& get_name() const { return m_info.name; }

    const std::string& get_filepath() const { return m_info.filepath; }

    bool is_valid() const { return m_font.data != nullptr; }

private: // methods
    const Size2i& get_atlas_size() const { return m_info.atlas_size; }

private: // fields
    /** stb_truetype internal Font object. */
    stbtt_fontinfo m_font;

    /** stb_truetype internal Font pack context. */
    stbtt_pack_context m_pack;

    /** FontInfo object used to initialize the Font. */
    FontInfo m_info;

    /** Glyphs of this Font. */
    std::vector<Glyph> m_glyphs;

    /** Height of a lower-case letter above the median of the Font.
     * See https://en.wikipedia.org/wiki/Ascender_(typography)
     */
    float m_ascender;

    /** Height of a lower-case letter below the baseline of the Font.
     * See https://en.wikipedia.org/wiki/Descender
     */
    float m_descender;

    /** Vertical distance of two lines within the same paragraph. */
    float m_line_height;
};

} // namespace notf
