#pragma once

#include "common/xform2.hpp"
#include "core/claim.hpp"
#include "core/item.hpp"

namespace notf {

class CharEvent;
class KeyEvent;
class FocusEvent;
class MouseEvent;

class RenderLayer;
using RenderLayerPtr = std::shared_ptr<RenderLayer>;

/**********************************************************************************************************************/

/**
 *
 * Claims
 * ================
 * All ScreenItems have a Claim, that is a minimum / preferred / maximum 2D size that it would like to occupy on screen,
 * but Layouts and Widgets use them in different ways:
 *
 * For a Widget, the Claim is a hard constraint.
 * It can never shrink beyond its minimum and never grow beyond its maximum.
 * Everything in between is determined by its parent Layout, which should have a tendency to favor the Widget's
 * preferred size if in doubt.
 *
 * Layouts have two modes in which they can operate.
 * By default, a Layout builds up its Claim by accumulating all of its items' Claims in a layout-specific way.
 * In that case, the combined minimum size of all Items becomes the Layout's minimum size - the same goes for the
 * maximum size.
 * Optionally, you can set an explicit Claim on the Layout, which internally causes the Layout to ignore its Items'
 * Claims and provide its own from now on.
 * This way, you can have a scroll area that takes up available space and has its size set in response to its own Claim,
 * rather than to the combined Claims of all of its child Items.
 * If you want to revert to an Item-driven Claim, call `set_claim` with a zero Claim.
 *
 * Layout negotiation
 * ==================
 * Layouts and Widgets need to "negotiate" the Layout.
 * Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
 * If its Claim changes, its respective parent Layout might need to update as well - up to the first Layout that does
 * not update its Claim (at the latest, the WindowLayout never updates its Claim).
 *
 * The pipeline is as follows:
 *
 *      1. A ScreenItem changes its Claim. Either a Widget claims more/less space in response to an event, a Layout
 *         finds itself with one more child or whatever.
 *      2. The ScreenItem notifies its parent Layout, which in turn updates its Claim and notifies its own parent.
 *         This chain continues, until one Layout finds, that its own Claim did not change after recalculation.
 *      3. The last notified Layout will re-layout all of its children and assign each one a new size and transform.
 *         Layout children will react by themselves re-layouting and potentially resizing their own children.
 *
 * Size
 * ====
 * The size is the visual size of the Widget in untransformed space.
 * If a Widget has a size of 100x100 in an unscaled space, it does not necessarily mean that the Widget takes up
 * 100x100 px in the Window as it might be transformed and / or scissored.
 *
 * A Layout's size is the area in which it can arrange its children.
 * There is no guarantee that all of the children will fit into the size and Layouts will frequently flow over, but
 * the size is the goal that the Layout should fulfill as closely as possible.
 *
 * A Widget's size is the size of its Cell, the area into which it is painted.
 * Like Layouts, Widgets are not bound to their size and can paint into the area outside as well.
 *
 * Spaces
 * ======
 *
 * Untransformed space
 * -------------------------
 * Claims are made in untransformed space.
 * That means, they are not affected by the local transform applied to the ScreenItem, nor do they change when the
 * parent Layout changes the ScreenItem's layout transform.
 * The ScreenItem's size is in this space also.
 *
 * Local (offset) space
 * --------------------
 * Each ScreenItem has full control over its own offset.
 * The offset is applied last and does not influence how the Layout perceives the ScreenItem, meaning if you scale the
 * ScreenItem twofold, it will appear bigger on screen but the scale will remain invisible to the the parent Layout.
 * That also means that clicking the cursor into the overflow areas will not count as a click inside the ScreenItem,
 * because the parent won't know that it appears bigger on screen.
 * Offsets are useful, for example, to apply a jitter animation or similiar transformations that should not affect the
 * layout.
 *
 * Layout (parent) space
 * ---------------------
 * Transformation controlled by the parent Layout.
 * Used mostly to position the ScreenItem within the parent Layout.
 * Can also be used as a projection matrix in a scene view ...?
 *
 * Opacity
 * =======
 * Each ScreenItem has an `opacity` member, which is a float in the range [0 -> 1].
 * An opacity of `0` means that the Item is fully transparent (invisible, in fact), an opacity of `0.5` semi-transparent
 * and `1` not transparent at all.
 * Opacity trickles down the hierarchy, meaning that in order to get to the effective opacity of an Item, you have to
 * multiply it's own opacity with that of each ancestor.
 *
 * Scissoring
 * ==========
 * In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
 * those Widgets need to be "scissored" by the scroll area.
 * A "Scissor" is an axis-aligned rectangle, scissoring is the act of cutting off parts of a Widget that fall outside
 * that rectangle.
 * Every Widget contains a pointer to the ancestor Layout that acts as its scissor.
 * By default, all ScreenItems are scissored to the WindowLayout, but you can explicitly override the scissor Layout for
 * each ScreenItem individually.
 * If a ScreenItem is moved outside of its scissor hierarchy, it will fall back to its parent's scissor Layout.
 * ScreenItems outside a hierarchy do not have a scissor.
 *
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
 *
 * Events
 * ======
 * All ScreenItems can handle events.
 * Events are created by the Application in reaction to something happening, like a user input or a system event.
 * Only Widgets receive events, which means that in order to handle events, a Layout must contain an invisible Widget
 * in the background (see ScrollArea for an example).
 * If a Widget receives an event but does not handle it, it is propagated up the ancestry until it either passes the
 * root or an ancestor Layout sets its `is_handled` flag.
 */
class ScreenItem : public Item {
    friend class RenderManager;
    friend class WindowLayout;

protected: // constructor *********************************************************************************************/
    ScreenItem(ItemContainerPtr container);

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

    /** Returns the axis-aligned bounding rect of this ScreenItem in parent space. */
    Aabrf get_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem as transformed by its layout only. */
    Aabrf get_layout_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem in local space. */
    Aabrf get_local_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem in Window space. */
    Aabrf get_window_aarbr() const;

    /** Returns the actual axis-aligned bounding rect of this ScreenItem in non-transformed space.
     * Widgets simply return their size, Layouts may have more involved methods of determining their effective Aabr.
     * Note that the Aabr may be larger or smaller than the ScreenItem size, because Layouts may need to accomodate
     * more children than their size would allow.
     */
    virtual Aabrf get_untransformed_aabr() const = 0;

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

    /** Sets the visibility flag of this ScreenItem.
     * Note that the ScreenItem is not guaranteed to be visible just because the visibility flag is true (see
     * `is_visible` for details).
     * If the flag is false however, the ScreenItem is guaranteed to be not visible.
     */
    void set_visible(bool is_visible);

    /** Returns the Layout used to scissor this ScreenItem. */
    const Layout* get_scissor() const { return m_scissor_layout; }

    /** Whether this ScreenItem will inherit its scissor Layout from its parent or supply its own. */
    bool has_explicit_scissor() const { return m_has_explicit_scissor; }

    /** Sets the new scissor Layout for this ScreenItem. */
    void set_scissor(const Layout* scissor_layout);

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

    /** Emitted when the visibility flag was changed by the user.
     * See `set_visible()` for details.
     */
    Signal<bool> on_visibility_changed;

    /** Emitted, when the opacity of this ScreenItem has changed.
     * Note that the effective opacity of a ScreenItem is determined through the multiplication of all of its ancestors
     * opacity.
     * If an ancestor changes its opacity, only itself will fire this signal.
     * @param New visiblity.
     */
    Signal<float> on_opacity_changed;

    /** Emitted when the scissor of this ScreenItem changed.
     */
    Signal<const Layout*> on_scissor_changed;

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
    virtual void _update_from_parent() override;

    /** Tells the Window that this ScreenItem needs to be redrawn.
     * @returns False, if the ScreenItem did not trigger a redraw because it is invisible.
     */
    bool _redraw() const;

    /** Unscaled size of this ScreenItem in local space. */
    const Size2f& _get_size() const { return m_size; }

    /** Recursive implementation to find all Widgets at a given position in local space
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const = 0;

    /** Updates the Claim of this Item, may also change its size to comply with the new constraints.
     * @return      True iff the Claim was modified.
     */
    bool _set_claim(const Claim claim);

    /** Updates the size of the ScreenItem.
     * Note that the ScreenItem's claim is a hard constraint, which is enforced by this method.
     * That means when you assign a size that cannot fulfill the claim, the actual size that ends up being set is not
     * the one passed into the function.
     * This function is virtual because Layouts use it to determine how to arrange their children.
     * @returns Whether or not the ScreenItem's size was updated or not.
     */
    virtual bool _set_size(const Size2f size) = 0;

    /** Updates the layout transformation of this Item. */
    void _set_layout_transform(const Xform2f transform);

    /** Sets a new Scissor for this ScreenItem. */
    void _set_scissor(const Layout* scissor_layout);

    /** Sets a new RenderLayer for this ScreenItem. */
    void _set_render_layer(const RenderLayerPtr& render_layer);

    /** Notifies the parent Layout that the Claim of this ScreenItem has changed.
     * The change propagates up the Item hierarchy until it reaches the first ancestor, that doesn't need to change its
     * Claim, where it proceeds downwards again to re-layout all changed Items.
     */
    void _update_ancestor_layouts();

protected: // static methods ******************************************************************************************/
    /** Allows ScreenItem subclasses to query Widgets from each other. */
    static void _get_widgets_at(const ScreenItem* screen_item, const Vector2f& local_pos, std::vector<Widget*>& result)
    {
        screen_item->_get_widgets_at(local_pos, result);
    }

    /** Allows ScreenItem subclasses to resize each other. */
    static bool _set_size(ScreenItem* screen_item, const Size2f size)
    {
        return screen_item->_set_size(std::move(size));
    }

    /** Allows ScreenItem subclasses to change each other's layout transformation. */
    static void _set_layout_transform(ScreenItem* screen_item, const Xform2f transform)
    {
        return screen_item->_set_layout_transform(std::move(transform));
    }

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

    /** Unscaled size of this ScreenItem in local space.
     * Is an internal field that helps the ScreenItem to realize when its effective size has changed.
     * Use the Aabr getters to determine the 'real' size of the ScreenItem in the space of you choice.
     */
    Size2f m_size;

    /** Flag indicating whether a ScreenItem should be visble or not.
     * Note that the ScreenItem is not guaranteed to be visible just because this flag is true.
     * If the flag is false however, the ScreenItem is guaranteed to be invisible.
     */
    bool m_is_visible;

    /** Opacity of this ScreenItem in the range [0 -> 1]. */
    float m_opacity;

    /** Reference to a Layout in the ancestry, used to 'scissor' this ScreenItem. */
    const Layout* m_scissor_layout;

    /** Whether this ScreenItem will inherit its scissor Layout from its parent or supply its own.
     * If the ScreenItem provides its own scissor and is moved out of its scissor's hierarchy, this flag is reset to
     * false, causing the ScreenItem to inherit its new parent's scissor instead.
     */
    bool m_has_explicit_scissor;

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
Xform2f transformation_between(const ScreenItem* source, const ScreenItem* target);

} // namespace notf
