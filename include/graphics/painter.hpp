#pragma once

#include "graphics/texture2.hpp"
#include "common/vector2.hpp"

namespace notf {

class Aabr;
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

    void drawSlider(const Aabr& rect, float pos);

    void drawButton(const Aabr& rect);
    void drawCheckBox(const Aabr& rect);
    void drawColorwheel(const Aabr& rect, float t);
    void drawEyes(const Aabr& rect, const Vector2f& target, float t);
    void drawGraph(const Aabr& rect, float t);
    void drawSpinner(const Vector2f& center, const float radius, float t);
    void drawCaps(const Vector2f& pos, const float width);
    void drawJoins(const Aabr& rect, const float time);
    void drawTexture(const Aabr& rect);

private: // fields
    const Widget& m_widget;

    Cell& m_cell;

    RenderContext& m_context;

    static std::shared_ptr<Texture2> test_texture;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
