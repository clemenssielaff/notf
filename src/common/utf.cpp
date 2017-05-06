#include "common/utf.hpp"

namespace notf {

std::ostream& operator<<(std::ostream& out, const Codepoint codepoint)
{
    if (codepoint.value <= 0x7f) {
        out << static_cast<char>(codepoint.value);
    }
    else if (codepoint.value <= 0x7ff) {
        out << static_cast<char>(0xc0 | ((codepoint.value >> 6) & 0x1f));
        out << static_cast<char>(0x80 | (codepoint.value & 0x3f));
    }
    else if (codepoint.value <= 0xffff) {
        out << static_cast<char>(0xe0 | ((codepoint.value >> 12) & 0x0f));
        out << static_cast<char>(0x80 | ((codepoint.value >> 6) & 0x3f));
        out << static_cast<char>(0x80 | (codepoint.value & 0x3f));
    }
    else {
        out << static_cast<char>(0xf0 | ((codepoint.value >> 18) & 0x07));
        out << static_cast<char>(0x80 | ((codepoint.value >> 12) & 0x3f));
        out << static_cast<char>(0x80 | ((codepoint.value >> 6) & 0x3f));
        out << static_cast<char>(0x80 | (codepoint.value & 0x3f));
    }
    return out;
}

} // namespace notf
