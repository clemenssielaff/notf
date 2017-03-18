#include "graphics/vertex.hpp"

#include <iostream>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Vertex& vert)
{
    return out << "Vertex(pos: " << vert.pos << ", uv: " << vert.uv << ")";
}

static_assert(sizeof(Vertex) == sizeof(Vector2f) * 2,
              "This compiler seems to inject padding bits into the notf::Vertex memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vertex>::value,
              "This compiler does not recognize notf::Vertex as a POD.");

} // namespace notf
