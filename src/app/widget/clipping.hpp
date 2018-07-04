#pragma once

#include "common/aabr.hpp"
#include "common/matrix3.hpp"
#include "common/polygon.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Clipping rect with transformation.
class Clipping {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Clipping rectangle centered around the transformation.
    const Aabrf& get_rect() const { return m_rect; }

    /// Update the Clipping rect.
    void set_rect(Aabrf rect) { m_rect = std::move(rect); }

    /// Transformation of the Clipping Rectangle.
    const Matrix3f& get_xform() const { return m_xform; }

    /// Update the Clipping's transformation.
    void set_xform(Matrix3f xform) { m_xform = std::move(xform); }

    /// Polygon to clip to, can be empty.
    const Polygonf& get_polygon() const { return m_polygon; }

    /// Update the clipping polygon.
    void set_polygon(Polygonf polygon)
    {
        m_polygon = std::move(polygon);
        // TODO: constraint clipping Polygon to rect, or rect to polygon?
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Polygon to clip to, can be empty.
    /// Must be contained within `rect`.
    Polygonf m_polygon;

    /// Transformation of the Clipping Rectangle.
    Matrix3f m_xform;

    /// Clipping rectangle centered around the transformation.
    Aabrf m_rect;
};

NOTF_CLOSE_NAMESPACE
