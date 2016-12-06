#pragma once

#include <memory>
#include <set>
#include <vector>

namespace notf {

class LayoutItem;
struct RenderContext;
class Widget;
class Window;

/**********************************************************************************************************************/

/**
 * @brief The RenderLayer class
 */
class RenderLayer {

    friend class RenderManager;

protected: // methods
    RenderLayer() = default;

private: // fields
    /** Widgets ordered from back to front. */
    std::vector<const Widget*> m_widgets;
};

/**********************************************************************************************************************/

/**
 * @brief The RenderManager class
 */
class RenderManager {

protected: // methods
    explicit RenderManager(const Window* window);

public: // methods
    /** Checks, whether there are any LayoutItems that need to be redrawn. */
    bool is_clean() const { return m_is_clean; }

    /** Returns the default RenderLayer that always exists. */
    std::shared_ptr<RenderLayer> get_default_layer() const { return m_default_layer; }

    /** Creates and returns a new RenderLayer at the very front of the stack. */
    std::shared_ptr<RenderLayer> create_front_layer();

    /** Creates and returns a new RenderLayer at the very back of the stack. */
    std::shared_ptr<RenderLayer> create_back_layer();

    /** Creates and returns a new RenderLayer directly above the given one. */
    std::shared_ptr<RenderLayer> create_layer_above(const std::shared_ptr<RenderLayer>& layer);

    /** Creates and returns a new RenderLayer directly below the given one. */
    std::shared_ptr<RenderLayer> create_layer_below(const std::shared_ptr<RenderLayer>& layer);

    /** Sets the RenderManager dirty so it redraws on the next frame. */
    void request_redraw() { m_is_clean = false; }

    /** Renders all registered Widgets in their correct z-order.
     * Cleans the RenderManager and doesn't render if clean to begin with.
     * @param context   The context into which to render.
     */
    void render(const RenderContext& context);

private: // methods
    void _iterate_layout_hierarchy(const LayoutItem* layout_item, RenderLayer* parent_layer);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** The default layer, will never go out of scope as long as the RenderManager lives. */
    std::shared_ptr<RenderLayer> m_default_layer;

    /** All Render layers. */
    std::vector<std::shared_ptr<RenderLayer>> m_layers;

    /** Whether the RenderManager needs to update. */
    bool m_is_clean;
};

} // namespace notf
