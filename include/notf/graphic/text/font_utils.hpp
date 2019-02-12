#include <string>
#include <vector>

#include "notf/common/geo/aabr.hpp"
#include "notf/common/utf8.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

/// Returns the aabr for a given text rendered with a given font.
/// The aabr origin is at the first glyph's origin.
/// @param font  Font used to render the text.
/// @param text  Text to render.
/// @return      Aabr of the text when rendered in one line.
Aabri text_aabr(const FontPtr& font, const std::string& text);

/// Breaks a text into several lines at the delimiter character.
/// @param width     Targeted width.
/// @param font      Font used to size the text.
/// @param text      Text to split.
/// @param first     Index of the utf8 character in the string to start splitting.
/// @param limit     Maximum number of breaks. -1 means no limit.
/// @param delimiter Delimiter character. Can be zero in which case the line break is immediate.
/// @return          One iterator for each line.
std::vector<std::string::const_iterator>
break_text(const int width, const FontPtr& font, const std::string& text, const size_t first = 0, const int limit = -1,
           const char32_t delimiter = ' ');

NOTF_CLOSE_NAMESPACE
