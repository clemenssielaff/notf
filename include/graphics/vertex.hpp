#pragma once

#include <iosfwd>

#include "common/hash.hpp"
#include "common/vector2.hpp"

namespace notf {

//*********************************************************************************************************************/

/** A 2D Vertex with position and uv-coordinates, can be used in an OpenGL buffer. */
struct Vertex {

    /** Position of the Vertex. */
    Vector2f pos;

    /** UV-coordinates of the Vertex. */
    Vector2f uv;

    /** Default (non-initializing) constructor so this struct remains a POD */
    Vertex() = default;

    /** Value Constructor. */
    Vertex(Vector2f pos, Vector2f uv)
        : pos(std::move(pos)), uv(std::move(uv)) {}
};

/**********************************************************************************************************************/

/** Prints the contents of a Vertex into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param vert Vertex to print.
 * @return Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Vertex& vert);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Vertex. */
template <>
struct hash<notf::Vertex> {
    size_t operator()(const notf::Vertex& vertex) const { return notf::hash(vertex.pos, vertex.uv); }
};

} // namespace std
