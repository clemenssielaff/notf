#pragma once

#include "core/component.hpp"

namespace notf {

class Widget;
class Painter;
struct RenderContext;

/**********************************************************************************************************************/

/** The Canvas Component contains information on how to draw the Widget. */
class CanvasComponent : public Component {

public: // methods
    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::CANVAS; }

    /** Draws this Canvas with the given Widget and Render context. */
    void render(const Widget& widget, const RenderContext& context);

    /** Defines a new painter object to paint on this canvas. */
    void set_painter(std::shared_ptr<Painter> painter) { m_painter = std::move(painter); }

protected: // methods
    /// @brief Default Constructor.
    explicit CanvasComponent() = default;

private: // fields
    /** Painter object used to draw on the canvas. */
    std::shared_ptr<Painter> m_painter;
};

} // namespace notf
