#pragma once

#include "graphics/cell/command_buffer.hpp"

namespace notf {

class Cell {

    friend class Painter;

public: // methods
    Cell() = default;

    /** The Painter Command buffer of this Cell. */
    const PainterCommandBuffer& get_commands() const { return m_commands; }

private: // fields
    PainterCommandBuffer m_commands;
};

} // namespace notf
