#pragma once

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

private: // fields
    const Widget& m_widget;

    Cell& m_cell;

    RenderContext& m_context;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
