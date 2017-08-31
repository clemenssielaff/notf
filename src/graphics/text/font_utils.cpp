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
break_text(const int width, const FontPtr& font, const std::string& text, const size_t first,
           const int limit, const Codepoint delimiter)
{
    std::vector<std::string::const_iterator> result;

    int advance                 = 0;
    int word_advance            = 0;
    size_t last_delimiter_index = 0;
    const utf8_string utf8_text(text);
    for (auto it = std::begin(utf8_text) + static_cast<std::string::difference_type>(first);
         it != std::end(utf8_text); ++it) {
        const Glyph& glyph = font->glyph(static_cast<codepoint_t>(*it));

        if (delimiter.value == 0 || *it == delimiter.value) {
            auto one_step_further = it; // include the delimiter
            last_delimiter_index  = static_cast<size_t>((++one_step_further).get_index());
            word_advance          = 0;
        }
        else {
            word_advance += glyph.advance_x;
        }

        const int new_advance = advance + glyph.advance_x;
        if (last_delimiter_index && new_advance > width) {

            // insert a new line break
            std::string::const_iterator linebreak = std::begin(text);
            std::advance(linebreak, last_delimiter_index);
            result.emplace_back(linebreak);

            // return early when hitting the limit
            if (limit > 0 && result.size() == static_cast<size_t>(limit)) {
                return result;
            }

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
