#include "dynamic/widget/textwidget.hpp"

#include "graphics/cell/painter.hpp"
#include "graphics/text/font.hpp"
#include "graphics/text/font_utils.hpp"

namespace notf {

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
        m_line_heights.clear();
        m_line_heights.push_back(aabr.get_height());

        Claim claim;
        claim.set_min(aabr.get_width(), aabr.get_height());
        _set_claim(std::move(claim));
    }
}

void TextWidget::_relayout()
{
    Widget::_relayout();

    if (!m_font || !m_font->is_valid()) {
        return;
    }

    if (m_is_wrapping) {
        m_newlines = split_text_by_with(
            static_cast<int>(std::ceil(get_size().width)), static_cast<utf32_t>(' '), m_font, m_text);

        m_line_heights.clear();
        for (size_t i = 0; i < m_newlines.size(); ++i) {
            const std::string::const_iterator begin = m_newlines[i];
            const std::string::const_iterator end   = i == m_newlines.size() - 1 ? std::end(m_text) : m_newlines[i + 1];
            const std::string substring(begin, end); // TODO: too much string copying (also below)

            const Aabri aabr = text_aabr(m_font, substring);
            m_line_heights.push_back(aabr.get_height());
        }
    }
}

void TextWidget::_paint(Painter& painter) const
{
    if (!m_font || !m_font->is_valid()) {
        return;
    }
    assert(!m_line_heights.empty());

    painter.set_fill(Color::black());

    if (m_is_wrapping) {
        assert(m_line_heights.size() == m_newlines.size());

        painter.translate(0, get_size().height);

        for (size_t i = 0; i < m_line_heights.size(); ++i) {
            const std::string::const_iterator begin = m_newlines[i];
            const std::string::const_iterator end   = i == m_newlines.size() - 1 ? std::end(m_text) : m_newlines[i + 1];
            const std::string substring(begin, end);

            painter.translate(0, -m_line_heights[i]);

            painter.write(substring, m_font);
        }
    }
    else {
        painter.translate(0, get_size().height - m_line_heights[0]);

        painter.write(m_text, m_font);
    }
}

} // namespace notf
