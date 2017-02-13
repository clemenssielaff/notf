#pragma once

namespace notf {

class Aabr;
class Cell;
class Widget;
class RenderContext;
class Vector2;

class Painter {



public: // methods
    Painter(const Widget& widget, Cell& cell, RenderContext& context)
        : m_widget(widget)
        , m_cell(cell)
        , m_context(context)
    {
    }

    void test();

    void drawSlider(const Aabr& rect, float pos);

    void drawButton(const Aabr& rect);
    void drawCheckBox(const Aabr& rect);
    void drawColorwheel(const Aabr& rect, float t);
    void drawEyes(const Aabr &rect, const Vector2 &target, float t);
    void drawGraph(const Aabr& rect, float t);
    void drawSpinner(const Vector2& center, const float radius, float t);
    void drawCaps(const Vector2& pos, const float width);
    void drawJoins(const Aabr& rect, const float time);

private: // fields
    const Widget& m_widget;

    Cell& m_cell;

    RenderContext& m_context;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
