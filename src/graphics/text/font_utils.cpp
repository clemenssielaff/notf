#include "graphics/text/font_utils.hpp"

#include "graphics/text/font.hpp"

namespace notf {

Aabri text_aabr(const FontPtr& font, const std::string& text)
{
    Aabri result    = Aabri::zero();
    result._min.x() = 0;

    Aabri::value_t advance = 0;
    const utf8_string utf8_text(text);
    for (const auto character : utf8_text) {
        const Glyph& glyph = font->glyph(static_cast<codepoint_t>(character));

        result._min.y() = min(result._min.y(), glyph.rect.height - glyph.top);
        result._max.y() = max(result._max.y(), glyph.top);

        advance += glyph.advance_x;
        result._max.x() = advance;
    }

    return result;
}

std::vector<std::string::const_iterator>
split_text_by_with(const int width, const Codepoint delimiter, const FontPtr& font, const std::string& text)
{
    std::vector<std::string::const_iterator> result;
    result.emplace_back(std::begin(text));

    int advance                 = 0;
    int word_advance            = 0;
    size_t last_delimiter_index = 0;
    const utf8_string utf8_text(text);
    for (auto it = std::begin(utf8_text); it != std::end(utf8_text); ++it) {
        const Glyph& glyph = font->glyph(static_cast<codepoint_t>(*it));

        if (delimiter.value == 0 || *it == delimiter.value) {
            last_delimiter_index = static_cast<size_t>(it.get_index()) + 1; // include the delimiter
            word_advance         = 0;
        }
        else {
            word_advance += glyph.advance_x;
        }

        const int new_advance = advance + glyph.advance_x;
        if (last_delimiter_index && new_advance > width) {
            std::string::const_iterator linebreak = std::begin(text);
            std::advance(linebreak, last_delimiter_index);
            result.emplace_back(linebreak);

            advance              = word_advance;
            word_advance         = 0;
            last_delimiter_index = 0;
        }
        else {
            advance = new_advance;
        }
    }

    return result;
}

} // namespace notf
