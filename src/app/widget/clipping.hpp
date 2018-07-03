#pragma once

#include "common/aabr.hpp"
#include "common/matrix3.hpp"
#include "common/optional.hpp"
#include "common/polygon.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Clipping rect with transformation.
struct Clipping {

    /// Clipping rectangle centered around the transformation.
    Aabrf rect;

    /// Transformation of the Clipping Rectangle.
    Matrix3f xform;

    /// Polygon to clip to, can be empty.
    /// Must be contained within `rect`.
//    notf::optional<Polygonf> polygon;
};

NOTF_CLOSE_NAMESPACE
