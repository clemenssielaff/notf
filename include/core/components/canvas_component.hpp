#pragma once

#include <functional>

#include "core/component.hpp"

namespace notf {

class Widget;
class Painter;
struct RenderContext;

/**********************************************************************************************************************/

/** The Canvas Component contains information on how to draw the Widget. */
class CanvasComponent : public Component {

public: // methods
    virtual ~CanvasComponent() override;

    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::CANVAS; }

    /** Draws this Canvas with the given Widget and Render context. */
    void render(const Widget& widget, const RenderContext& context);

    /** Sets a new function for painting the canvas. */
    void set_paint_function(std::function<void(Painter& painter)> func) { m_paint_func = func; }

protected: // methods
    /// @brief Default Constructor.
    explicit CanvasComponent()
        : m_paint_func()
    {
    }

private: // fields
    /** Paint function. */
    std::function<void(Painter& painter)> m_paint_func;
};

} // namespace notf
