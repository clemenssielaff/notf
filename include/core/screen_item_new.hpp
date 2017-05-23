#pragma once

#include "common/claim.hpp"
#include "common/xform2.hpp"
#include "core/item_new.hpp"

namespace notf {

class CharEvent;
class KeyEvent;
class FocusEvent;
class MouseEvent;

class RenderLayer;
using RenderLayerPtr = std::shared_ptr<RenderLayer>;

/**
 * RenderLayer
 * ===========
 * By default, it is the Layouts' job to determine the order in which Widgets are drawn on screen.
 * However, we might want to make exceptions to this rule, where an Widget (for example a tooltip) is logically part of
 * a nested Layout, but should be drawn on top of everything else.
 * For that, we have RenderLayers, explicit layers that each ScreenItem in the hierarchy can be assigned to in order to
 * render them before or after other parts of the hierarchy.
 * The WindowLayout is part of the default RenderLayer `zero`.
 * If you set an ScreenItem to another RenderLayer (for example `one`) it, and all of its children will be drawn in
 * front of everything in RenderLayer zero.
 */
class ScreenItem : public Item {
protected: // constructor *********************************************************************************************/
    ScreenItem(std::unique_ptr<detail::ItemContainer> container);

public: // methods ****************************************************************************************************/
    /** Returns the ScreenItem's effective transformation in parent space. */
    const Xform2f& get_transform() const { return m_effective_transform; }

    /** 2D transformation of this ScreenItem as determined by its parent Layout. */
    const Xform2f& get_layout_transform() const { return m_layout_transform; }

    /** 2D transformation of this ScreenItem on top of the layout transformation. */
    const Xform2f& get_local_transform() const { return m_local_transform; }

    /** Recursive implementation to produce the ScreenItem's transformation in window space. */
    Xform2f get_window_transform() const;

    /** Updates the transformation of this ScreenItem. */
    void set_local_transform(const Xform2f transform);

    /** Returns the unscaled size of this ScreenItem in pixels. */
    const Size2f& get_size() const { return m_size; }

    /** Returns the axis-aligned bounding rect of this ScreenItem in parent space. */
    Aabrf get_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem as transformed by its layout only. */
    Aabrf get_layout_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem in local space. */
    Aabrf get_local_aarbr() const;

    /** The current Claim of this Item. */
    const Claim& get_claim() const { return m_claim; }

    /** Returns the effective opacity of this ScreenItem in the range [0 -> 1].
     * @param effective By default, the returned opacity will be the product of this ScreenItem's opacity with all of
     *                  its ancestors'. If set to false, the opacity of this ScreenItem alone is returned.
     */
    float get_opacity(bool effective = true) const;

    /** Sets the opacity of this ScreenItem.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     */
    void set_opacity(float opacity);

    /** Checks, if the ScreenItem is currently visible.
     * This method does return false if the opacity is zero but also if there are any other factors that make this
     * ScreenItem not visible, like a zero size for example or being completely scissored.
     */
    bool is_visible() const;

    /** The RenderLayer that this ScreenItem is a part of. */
    const RenderLayerPtr& get_render_layer() const { return m_render_layer; }

    /** Tests whether this ScreenItem has its own RenderLayer, or if it inherits one from its parent. */
    bool has_explicit_render_layer() const { return m_has_explicit_render_layer; }

    /** (Re-)sets the RenderLayer of this ScreenItem.
     * Pass an empty shared_ptr to implicitly inherit the RenderLayer from the parent Layout.
     * @param render_layer  New RenderLayer of this ScreenItem
     */
    void set_render_layer(const RenderLayerPtr& render_layer);

public: // signals ****************************************************************************************************/
    /** Emitted, when the size of this ScreenItem has changed.
     * @param New size.
     */
    Signal<const Size2f&> on_size_changed;

    /** Emitted, when the effective transform of this ScreenItem has changed.
     * @param New local transform.
     */
    Signal<const Xform2f&> on_transform_changed;

    /** Emitted, when the opacity of this ScreenItem has changed.
     * Note that the effective opacity of a ScreenItem is determined through the multiplication of all of its ancestors
     * opacity.
     * If an ancestor changes its opacity, only itself will fire this signal.
     * @param New visiblity.
     */
    Signal<float> on_opacity_changed;

    /** Emitted when the ScreenItem is moved into a new RenderLayer.
     * @param New RenderLayer.
     */
    Signal<const RenderLayerPtr&> on_render_layer_changed;

    /** Signal invoked when this ScreenItem is asked to handle a Mouse move event. */
    Signal<MouseEvent&> on_mouse_move;

    /** Signal invoked when this ScreenItem is asked to handle a Mouse button event. */
    Signal<MouseEvent&> on_mouse_button;

    /** Signal invoked when this ScreenItem is asked to handle a scroll event. */
    Signal<MouseEvent&> on_mouse_scroll;

    /** Signal invoked, when this ScreenItem is asked to handle a key event. */
    Signal<KeyEvent&> on_key;

    /** Signal invoked, when this ScreenItem is asked to handle a character input event. */
    Signal<CharEvent&> on_char_input;

    /** Emitted, when the ScreenItem has gained or lost the Window's focus. */
    Signal<FocusEvent&> on_focus_changed;

protected: // methods *************************************************************************************************/
    /** Sets the parent of this ScreenItem. */
    virtual void _set_parent(Item* parent) override;

    /** Sets a new RenderLayer for this ScreenItem. */
    void _set_render_layer(const RenderLayerPtr& render_layer);

private: // methods ***************************************************************************************************/
    /** Calculates the transformation of this ScreenItem relative to its Window. */
    void _get_window_transform(Xform2f& result) const;

    /** Updates the ScreenItem's effective transform if either the layout- or local transform changed. */
    void _update_effective_transform();

private: // fields ****************************************************************************************************/
    /** 2D transformation of this ScreenItem as determined by its parent Layout. */
    Xform2f m_layout_transform;

    /** 2D transformation of this ScreenItem on top of the layout transformation. */
    Xform2f m_local_transform;

    /** effective 2d transformation.
     * This value could be recalculated on-the-fly with `m_layout_transform * m_local_transform`, but usually it is
     * changed once and read many times which is why we store it.
     */
    Xform2f m_effective_transform;

    /** The Claim of a ScreenItem determines how much space it receives in the parent Layout.
     * Claim values are in untransformed local space.
     */
    Claim m_claim;

    /** Unscaled size of this ScreenItem in local space. */
    Size2f m_size;

    /** Opacity of this ScreenItem in the range [0 -> 1]. */
    float m_opacity;

    /** Reference to a Layout used to 'scissor' this Widget.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;

    /** The RenderLayer of this ScreenItem.
     * An empty pointer means that this ScreenItem inherits its RenderLayer from its parent.
     */
    RenderLayerPtr m_render_layer;

    /** Every ScreenItem references a render layer, but most implicitly inherit theirs from their parent.
     * If a RenderLayer is explicitly set, this flag is set to true, so moving the ScreenItem to another parent will not
     * change the RenderLayer.
     */
    bool m_has_explicit_render_layer;
};

/**********************************************************************************************************************/

/** Calculates a transformation from a given ScreenItem to another one.
 * @param source    ScreenItem prodiving source coordinates in local space.
 * @param target    ScreenItem into which the coordinates should be transformed.
 * @return          Transformation.
 * @throw           std::runtime_error, if the two ScreenItems do not share a common ancestor.
 */
Xform2f transformation_between(const ScreenItem *source, const ScreenItem *target);

} // namespace notf
