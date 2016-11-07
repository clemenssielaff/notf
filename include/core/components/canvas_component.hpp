#pragma once

#include "core/component.hpp"

namespace notf {

class Widget;
struct RenderContext;

/**********************************************************************************************************************/

/** The Canvas Component contains information on how to draw the Widget. */
class CanvasComponent : public Component {

public: // methods
    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::CANVAS; }

    /** Draws this Canvas with the given Widget and Render context. */
    void render(const Widget& widget, const RenderContext& context);

protected: // methods
    /// @brief Default Constructor.
    explicit CanvasComponent() = default;

private: // fields

};

} // namespace notf
