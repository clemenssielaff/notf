#include <string>
#include <vector>

#include "common/aabr.hpp"
#include "common/utf.hpp"

namespace notf {

class Font;
using FontPtr = std::shared_ptr<Font>;

/** Returns the aabr for a given text rendered with a given font.
 * The aabr origin is at the first glyph's origin.
 * @param font  Font used to render the text.
 * @param text  Text to render.
 * @return      Aabr of the text when rendered in one line.
 */
Aabri text_aabr(const FontPtr& font, const std::string& text);

/** Breaks a text into several lines at the delimiter character.
 * @param width     Targeted width.
 * @param delimiter Delimiter character. Can be zero in which case the line break is immediate.
 * @param font      Font used to size the text.
 * @param text      Text to split.
 * @return          One iterator for each line.
 */
std::vector<std::string::const_iterator>
split_text_by_with(const int width, const Codepoint delimiter, const FontPtr& font, const std::string& text);

} // namespace notf
