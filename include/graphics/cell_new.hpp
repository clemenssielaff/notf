#pragma once

#include "graphics/vertex.hpp"

namespace notf {

class Cell {

    friend class Painter;

    std::vector<Vertex> m_vertices;
};

} // namespace notf
