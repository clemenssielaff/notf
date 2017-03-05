#pragma once

#include "common/aabr.hpp"
#include "common/vector2.hpp"
#include "graphics/texture2.hpp"

namespace notf {

class Cell;
class Widget;
class RenderContext;

class Painter {

public: // methods
    Painter(const Widget& widget, Cell& cell, RenderContext& context)
        : m_widget(widget)
        , m_cell(cell)
        , m_context(context)
    {
    }

    void test();

    void drawSlider(const Aabrf& rect, float pos);

    void drawButton(const Aabrf& rect);
    void drawCheckBox(const Aabrf& rect);
    void drawColorwheel(const Aabrf& rect, float t);
    void drawEyes(const Aabrf& rect, const Vector2f& target, float t);
    void drawGraph(const Aabrf& rect, float t);
    void drawSpinner(const Vector2f& center, const float radius, float t);
    void drawCaps(const Vector2f& pos, const float width);
    void drawJoins(const Aabrf& rect, const float time);
    void drawTexture(const Aabrf& rect);

private: // fields
    const Widget& m_widget;

    Cell& m_cell;

    RenderContext& m_context;

    static std::shared_ptr<Texture2> test_texture;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
