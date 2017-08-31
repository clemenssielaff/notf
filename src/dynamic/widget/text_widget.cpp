#include "dynamic/widget/text_widget.hpp"

#include "dynamic/widget/capability/text_capability.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/text/font.hpp"
#include "graphics/text/font_utils.hpp"

// TODO: make sure that all file names are similar (before, this was textwidget.cpp)

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
    TextCapabilityPtr text_capability = std::make_shared<TextCapability>();
    text_capability->font             = m_font;
    set_capability(std::move(text_capability));

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
        claim.set_min(aabr.get_width(), m_font->line_height() * m_line_height);
        _set_claim(std::move(claim));
    }
}

void TextWidget::_relayout()
{
    Size2f size = get_claim().apply(get_grant());

    if (!m_font || !m_font->is_valid()) {
        return;
    }

    TextCapabilityPtr text_capability = capability<TextCapability>();

    if (m_is_wrapping) {
        m_newlines.clear();
        m_newlines.emplace_back(std::begin(m_text));
        const float first_width = size.width - text_capability->baseline_start.x();
        auto first_break        = break_text(static_cast<int>(first_width), m_font, m_text, /*first*/ 0, /*limit*/ 1);
        if (!first_break.empty()) {
            m_newlines.emplace_back(first_break.front());
        }
        size_t first = static_cast<size_t>(std::distance(std::cbegin(m_text), first_break.front()));
        for (auto newline : break_text(static_cast<int>(std::floor(size.width)), m_font, m_text, first)) {
            m_newlines.push_back(newline);
        }

        size.height = m_font->ascender() + m_font->descender()
            + ((m_newlines.size() - 1) * m_line_height * m_font->line_height());

        const std::string last_line(m_newlines.back(), std::cend(m_text));
        text_capability->baseline_end = Vector2f(
            text_aabr(m_font, last_line).get_width() - size.width,
            m_font->descender());
    }
    else { // not wrapping
        const Aabri line_aabr = text_aabr(m_font, m_text);

        size.height = line_aabr.get_height();
        size.width  = line_aabr.get_width();

        text_capability->baseline_end = Vector2f(0, m_font->descender());
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
    painter.translate(0, get_size().height);

    const float line_height = m_font->line_height() * m_line_height;
    if (m_is_wrapping) {
        float x = capability<TextCapability>()->baseline_start.x();
        for (size_t i = 0; i < m_newlines.size(); ++i) {
            const std::string::const_iterator begin = m_newlines[i];
            const std::string::const_iterator end   = i == m_newlines.size() - 1 ? std::end(m_text) : m_newlines[i + 1];
            const std::string substring(begin, end);

            painter.translate(x, -line_height);
            if (x > 0.0001) {
                x *= -1;
            }
            else if (x < -0.001) {
                x = 0;
            }

            painter.write(substring, m_font);
        }
    }
    else {
        painter.translate(capability<TextCapability>()->baseline_start);

        painter.write(m_text, m_font);
    }
}

} // namespace notf
