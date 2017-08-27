#include "core/widget.hpp"

#include "common/color.hpp"

namespace notf {

class Font;
using FontPtr = std::shared_ptr<Font>;

class TextWidget;
using TextWidgetPtr = std::shared_ptr<TextWidget>;

/**********************************************************************************************************************/

/** The TextWidget is a simple text widget, displaying a text in a single color and font.
 */
class TextWidget : public Widget {
public: // constructor ************************************************************************************************/
    TextWidget(FontPtr font, const Color color, const std::string text = "")
        : Widget()
        , m_text(std::move(text))
        , m_font(std::move(font))
        , m_color(color)
        , m_is_wrapping(false)
        , m_newlines()
        , m_line_heights()
    {
        _update_claim();
    }

    TextWidget(FontPtr font, const std::string text = "")
        : TextWidget(font, Color::black(), std::move(text))
    {
    }

public: // methods ****************************************************************************************************/
    /** Text displayed by this Widget. */
    const std::string& text() const { return m_text; }

    /** Change the text displayed by this Widget. */
    void set_text(const std::string text);

    /** Font used to render the text. */
    const FontPtr& font() const { return m_font; }

    /** Change the text's font. */
    void set_font(FontPtr font);

    /** Color of the text displayed by this Widget. */
    Color color() const { return m_color; }

    /** Change the text displayed by this Widget. */
    void set_color(const Color color);

    /** Whether the text is wrapped or not. */
    bool is_wrapping() const { return m_is_wrapping; }

    /** Changes whether the text wraps or not. */
    void set_wrapping(bool is_wrapping);

private: // methods ***************************************************************************************************/
    /** Updates the Claim of this Widget after the Font or text have changed. */
    void _update_claim();

    virtual void _relayout() override;

    virtual void _paint(Painter& painter) const override;

private: // fields ****************************************************************************************************/
    /** Text displayed in this widget. */
    std::string m_text;

    /** Font used to render the text. */
    FontPtr m_font;

    /** Color used to render the text. */
    Color m_color;

    /** Whether the text wraps or not. */
    bool m_is_wrapping;

    /** Iterator to newlines if the text is wrapping. */
    std::vector<std::string::const_iterator> m_newlines;

    /** Height of each line, if the text is wrapping. */
    std::vector<float> m_line_heights;
};

} // namespace notf
