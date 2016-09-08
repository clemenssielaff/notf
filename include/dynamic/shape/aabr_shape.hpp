#pragma once

#include "core/components/shape_component.hpp"

namespace signal {

/// \brief Simple, Axis Aligned Bounding Rect shape.
///
class AabrShapeComponent : public ShapeComponent {

public: // methods
    /// \brief Returns the Shape's Axis Aligned Bounding Rect.
    /// \param widget   Widget determining the shape.
    ///
    virtual Aabr get_aabr(const Widget& widget) override;

    /// \brief Sets a new AAbr.
    void set_aabr(Aabr aabr);

protected: // methods
    /// \brief Default Constructor.
    explicit AabrShapeComponent(Aabr aabr)
        : m_aabr(std::move(aabr))
    {
    }

private: // fields
    /// \brief The internal Aabr.
    Aabr m_aabr;
};

} // namespace signal
