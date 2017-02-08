#pragma once

namespace notf {

class Cell;
class RenderContext;

class Painter {

public: // methods
    Painter(Cell& cell, RenderContext& context)
        : m_cell(cell)
        , m_context(context)
    {
    }

    void test();

private: // fields
    Cell& m_cell;

    RenderContext& m_context;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
