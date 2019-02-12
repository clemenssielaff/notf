#pragma once

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/polygon.hpp"
#include "notf/common/matrix3.hpp"

NOTF_OPEN_NAMESPACE

// clipping ========================================================================================================= //

/// Clipping rect with transformation.
class Clipping {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Clipping.
    Clipping() = default;

    /// Construction from an Aabr only.
    Clipping(Aabrf aabr) : m_polygon(convert_to<Polygonf>(aabr)), m_xform(M3f::identity()), m_rect(std::move(aabr)) {}

    /// Clipping rectangle centered around the transformation.
    const Aabrf& get_rect() const { return m_rect; }

    /// Update the Clipping rect.
    void set_rect(Aabrf rect) { m_rect = std::move(rect); }

    /// Transformation of the Clipping Rectangle.
    const M3f& get_xform() const { return m_xform; }

    /// Update the Clipping's transformation.
    void set_xform(M3f xform) { m_xform = std::move(xform); }

    /// Polygon to clip to, can be empty.
    const Polygonf& get_polygon() const { return m_polygon; }

    /// Update the clipping polygon.
    void set_polygon(Polygonf polygon) {
        m_polygon = std::move(polygon);
        // TODO: constraint clipping Polygon to rect, or rect to polygon?
    }

    /// Equality comparison operator.
    /// @param other    Clipping to compare against.
    bool operator==(const Clipping& other) const noexcept {
        return m_rect.is_approx(other.m_rect)      //
               && m_xform.is_approx(other.m_xform) //
               && m_polygon.is_approx(other.m_polygon);
    }

    /// Inequality operator
    /// @param other    Clipping to compare against.
    constexpr bool operator!=(const Clipping& other) const noexcept { return !(*this == other); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Polygon to clip to, can be empty.
    /// Must be contained within `rect`.
    Polygonf m_polygon;

    /// Transformation of the Clipping Rectangle.
    M3f m_xform;

    /// Clipping rectangle centered around the transformation.
    Aabrf m_rect;
};

NOTF_CLOSE_NAMESPACE
