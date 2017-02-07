#pragma once

namespace notf {

class Cell;

class Painter {

public: // methods
    Painter(Cell& cell)
        : m_cell(cell)
    {
    }

private: // fields
    Cell& m_cell;
};

// TODO: move as much state from Cell to Painter in order to avoid growing the widget too large

} // namespace notf
