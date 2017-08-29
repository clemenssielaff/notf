#include "dynamic/widget/textwidget.hpp"

#include "graphics/cell/painter.hpp"
#include "graphics/text/font.hpp"
#include "graphics/text/font_utils.hpp"

// TODO: textwidget.cppp but text_layout.cpp ?

namespace notf {

TextWidget::TextWidget(FontPtr font, const Color color, const std::string text)
    : Widget()
    , m_text(std::move(text))
    , m_font(std::move(font))
    , m_color(color)
    , m_is_wrapping(false)
    , m_line_height(1)
    , m_newlines()
{
    _update_claim();
}

void TextWidget::set_text(const std::string text)
{
    if (text == m_text) {
        return;
    }
    m_text = std::move(text);
    _update_claim();
}

void TextWidget::set_font(FontPtr font)
{
    if (font == m_font) {
        return;
    }
    m_font = std::move(font);
    _update_claim();
}

void TextWidget::set_color(const Color color)
{
    if (color == m_color) {
        return;
    }
    m_color = color;
    _update_claim();
}

void TextWidget::set_wrapping(bool is_wrapping)
{
    if (is_wrapping == m_is_wrapping) {
        return;
    }
    m_is_wrapping = is_wrapping;
    _update_claim();
}

void TextWidget::set_line_height(const float line_height)
{
    if (abs(m_line_height - line_height) < precision_low<float>()) {
        return;
    }
    m_line_height = line_height;
    _update_claim();
}

void TextWidget::_update_claim()
{
    if (!m_font || !m_font->is_valid()) {
        return;
    }

    if (m_is_wrapping) {
        set_claim(Claim());
    }
    else {
        const Aabri aabr = text_aabr(m_font, m_text);

        Claim claim;
        claim.set_min(aabr.get_width(), m_font->pixel_size() * m_line_height);
        _set_claim(std::move(claim));
    }
}

void TextWidget::_relayout()
{
    Size2f size = get_claim().apply(get_grant());

    if (!m_font || !m_font->is_valid()) {
        return;
    }

    if (m_is_wrapping) {
        m_newlines = split_text_by_with(
            static_cast<int>(std::ceil(size.width)), static_cast<utf32_t>(' '), m_font, m_text);
        size.height = m_newlines.size() * m_line_height * m_font->pixel_size();
    }

    _set_size(size);
    _set_content_aabr(size);
}

void TextWidget::_paint(Painter& painter) const
{
    if (!m_font || !m_font->is_valid()) {
        return;
    }
    assert(!m_newlines.empty());

    painter.set_fill(m_color);

    const float line_height = m_font->pixel_size() * m_line_height;
    if (m_is_wrapping) {
        painter.translate(0, get_size().height);

        for (size_t i = 0; i < m_newlines.size(); ++i) {
            const std::string::const_iterator begin = m_newlines[i];
            const std::string::const_iterator end   = i == m_newlines.size() - 1 ? std::end(m_text) : m_newlines[i + 1];
            const std::string substring(begin, end);

            painter.translate(0, -line_height);

            painter.write(substring, m_font);
        }
    }
    else {
        painter.translate(0, get_size().height - line_height);

        painter.write(m_text, m_font);
    }
}

} // namespace notf