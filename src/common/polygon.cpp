#include "common/polygon.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

std::ostream& operator<<(std::ostream& out, const Polygonf& polygon)
{
    out << "Polygonf(";
    for (size_t i = 0; i < polygon.vertices.size(); ++i) {
        const auto& vertex = polygon.vertices[i];
        out << "(" << vertex.x() << ", " << vertex.y() << ")";
        if (i != polygon.vertices.size() - 1) {
            out << ", ";
        }
    }
    out << ")";
    return out;
}

NOTF_CLOSE_NAMESPACE
