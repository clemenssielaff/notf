#pragma once

#include <assert.h>
#include <memory>
#include <vector>

#include "common/size2.hpp"

namespace notf {

class RenderContext;
class RenderStats;
class ScreenItem;
class Widget;
class Window;

/**********************************************************************************************************************/

/** The RenderLayer class */
class RenderLayer {

    friend class RenderManager;

protected: // factory
    struct make_shared_enabler;

    static std::shared_ptr<RenderLayer> create();

    RenderLayer() = default;

private: // fields
    /** Widgets ordered from back to front. */
    std::vector<const Widget*> m_widgets;
};

/**********************************************************************************************************************/

/** The RenderManager class */
class RenderManager {

public: // methods
    /** Constructor */
    explicit RenderManager(const Window* window);

    /** Destructor. */
    ~RenderManager();

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
    void render(const Size2i buffer_size);

    /** Returns the index of a given RenderLayer, starting at 1. Returns 0 on failure. */
    size_t get_render_layer_index(const RenderLayer* render_layer)
    {
        assert(render_layer);
        for (size_t i = 0; i < m_layers.size(); ++i) {
            if (m_layers[i].get() == render_layer) {
                return i + 1;
            }
        }
        return 0;
    }

    /** Returns the RenderContext associated with the Window of this RenderManager. */
    RenderContext& get_render_context() const { return *m_render_context; }

private: // methods
    /** Iterates through all ScreenItems in the Item hierarchy and collects them in their RenderLayers. */
    void _iterate_item_hierarchy(const ScreenItem* screen_item, RenderLayer* parent_layer);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** The RenderContext used to draw into this Window. */
    std::unique_ptr<RenderContext> m_render_context;

    /** The default layer, will never go out of scope as long as the RenderManager lives. */
    std::shared_ptr<RenderLayer> m_default_layer;

    /** All Render layers. */
    std::vector<std::shared_ptr<RenderLayer>> m_layers;

    /** Whether the RenderManager needs to update. */
    bool m_is_clean;

    /** Render statistics debug display. */
    std::unique_ptr<RenderStats> m_stats;
};

} // namespace notf
