#pragma once

#include "common/aabr.hpp"
#include "core/item.hpp"

namespace notf {

class CharEvent;
class KeyEvent;
class FocusEvent;
class MouseEvent;

using uchar = unsigned char;

/** Baseclass for all Items that have physical expansion (Widgets and Layouts).
 *
 *
 *
 * ScreenItem size is an interesting topic, worth a little bit of discussion.
 * It is one of those things you don't really tend to think too much about, until you have to layout widgets on a screen.
 * Layouting is urprisingly hard, but you've heard that already, I suppose.
 * Let's go through a standard layouting process and how NoTF does it's magic behind the scenes.
 *
 * We'll look at Widgets first, and than at Layouts since Layouts contain Widgets but not the other way around.
 *
 * At the very least, a Widget needs a size: a 2-dimensional value describing the width and the height of the expansion
 * of the Widget on screen.
 * Widgets have a size and it does correspond to the screen area of the Widget if certain conditions are met.
 * For now, let's assume this is the case.
 * What determines the size of a Widget?
 * To a certain degree, the programmer.
 * You can set all Widget sizes explicitly and be done with it.
 * But that wouldn't make for a responsive interface.
 * As soon as you have basic Widget types, that we've all come to expect from our UIs (like splitter or resizeable
 * container), Widgets that only have a fixed size will end up taking too much screen real-estate or too litle.
 * It's like visiting an old website on your phone to find that there's no way to read a paragraph without horizontal
 * scrolling because the page does not respond to the size of you phone.
 *
 * This is why the Layout determines the size of a Widget, while the programmer only supplies a contraints and hints.
 * In NoTF, these are called a `Claim`.
 * Basically, a Widget claims a certain amount of space and then negotates with the Layout on how much it actually
 * receives.
 * You (the programmer) can provide a hard minimum and a hard maximum as well as a preferred value which is used to
 * distribute surplus space among multiple Widgets.
 * Additionally, you can define minimum and maximum width/height ratios for the area, as well as a priority that causes
 * Widgets with a higher priority to fill up as much space as allowed, before giving other Widgets the chance to do the
 * same.
 * If you want to make sure that the Widget has a certain size, you can set the min- and max-values of the Claim to the
 * same value.
 *
 * Up to this point, the size of the Widget is guaranteed to be in screen coordinates, meaning a size of 50x50 will
 * result in a Widget that takes up 50x50 pixels (on a monitor with a 1:1 relation between pixels and screen
 * coordinates, but that's a different topic).
 * But that is only true for unscaled Widgets.
 *
 * Widgets, like any ScreenItem, can be transformed.
 * If you don't know what that means, think "moved, rotated and scaled".
 * Moving is pretty straight forward and does not influence the size of the Widget.
 * Widgets are usually moved by their Layout, but you can always define a manual offset to a Widget's transform which
 * is added on top of the Layout's transformation.
 *
 * Rotating and scaling a Widget does influence its size and this is where things get a bit more complicated.
 * Fortunately, in most cases you will not need to use this functionality - but if you do, its nice to know that is
 * there.
 *
 * Transformations
 * ===============
 * As mentioned, you can always add an offset to a ScreenItem's transformation supplied by its Layout.
 * This works because internally, any ScreenItem has two transformations that are combined into its final transformation
 * in relation to its parent
 * Layout, which can be queried using `get_transform()`.
 * The `layout transform` is the transformation applied to the Item by its parent Layout.
 * The `local transform` is an additional offset transformation on the Item that is applied after the `layout
 * transform`.
 *
 *
 *
 *
 *
 * The thing is: transforming a Widget does not influence this (internal) size. Of course, a Widget scaled to twice its
 * size will appear bigger on screen, but its size data field (the one used by its Layout to place it) remains
 * unaffected.
 *
 * Let me repeat this, because this is important: scale and rotation of a Widget does not affect its Layout, nor does
 * it affect its hit detection.
 * Clicking into an area of a Widget that is outside its unscaled shape will not be registered.
 * Now, you might think that this makes things more complicated, but it really doesn't.
 * Scaling and rotating Widgets rarely occur in user interfaces and when it does, it is usally the 'Cell' of a Widget
 * that is rotating, not the Widget itself (see 'Cell' below).
 *
 *
 * Opacity
 * =======
 * Each ScreenItem has an `opacity` member, which is a float in the range [0 -> 1].
 * An opacity of `0` means that the Item is fully transparent (invisible, in fact), an opacity of `0.5` semi-transparent
 * and `1` not transparent at all.
 * Opacity trickles down the hierarchy, meaning that in order to get to the effective opacity of an Item, you have to
 * multiply it's own opacity with that of each ancestor.
 *
 * Size
 * ====
 * The size is the visual size of the Widget in parent space.
 * If a Widget has a size of 100x100 in an unscaled space, it does not necessarily mean that the Widget takes up
 * 100x100 px in the Window.
 * It might be outside the Window or (partly) scissored, but if you were to show it in its entirety, this is how large
 * it would be.
 * This is the same for both Widgets and Layouts.
 *
 * Claims
 * ================
 * Both widgets and Layouts have a Claim, that is a minimum / preferred / maximum 2D size that it would like to occupy
 * on the screen, but both use them in different ways:
 *
 * For a widget, the Claim is a hard constraint.
 * It can never shrink beyond its minimum and never grow beyond its maximum.
 * Everything in between is determined by its parent Layout, which should have a tendency to favor the widgets preferred
 * size if in doubt.
 *
 * Layouts have two modes in which they can operate.
 * By default, a Layout builds up its Claim by accumulating all of its items' Claims in a layout-specific way.
 * In that case, the combined minimum size of all Items becomes the Layout's minimum size - the same goes for the
 * maximum size.
 * Optionally, you can manually set a Claim, which internally causes the Layout to ignore its Items' Claims and provide
 * its own from now on.
 * This way, you can have a scroll area that takes up available space and has its size set in response to its own Claim,
 * rather than to the combined Claims of all of its child Items.
 * If you want to revert to an Item-driven Claim, call `set_claim` with a zero Claim.
 *
 * Layout negotiation
 * ==================
 * Layouts and Widgets need to "negotiate" the Layout.
 * Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
 * If its Claim changes, its respective parent might need to update as well - up to the first Layout that does not
 * update its Claim (at the latest, the WindowLayout never updates its Claim).
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
 *
 * Scissoring
 * ==========
 * In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
 * those Widgets need to be "scissored" by the scroll area.
 * A "Scissor" is an axis-aligned rectangle, scissoring is the act of cutting off parts of a Widget that fall outside
 * that rectangle.
 * Every Widget contains a pointer to the parent Layout that acts as its scissor.
 * An empty pointer means that this Widget is scissored by its parent Layout, but every Layout in this Widget's Item
 * ancestry can be used (including the Windows WindowLayout, which effectively disables scissoring).
 *
 * Spaces
 * ======
 *
 * Untransformed local space
 * -------------------------
 * Claims are made in untransformed local space.
 * That means, they are not affected by the local transform applied to the ScreenItem, nor do they change when the
 * parent Layout changes the ScreenItem's layout transform.
 *
 * Local (offset) space
 * --------------------
 * Each ScreenItem has full control over its own offset.
 * The offset is applied last and does not influence how the Layout perceives the ScreenItem, meaning if you scale the
 * ScreenItem twofold, it will appear bigger on screen but the scale will remain invisible to the the parent Layout.
 * That also means that clicking the cursor into the overflow areas will not count as a click inside the ScreenItem,
 * because the parent won't know that it appears bigger on screen.
 * Offsets are useful, for example, to apply a jitter animation to a Layout.
 *
 * Layout (parent) space
 * ---------------------
 * Transform controller by the parent Layout.
 * Used mostly to position the ScreenItem within the parent Layout.
 * Can also be used as a projection matrix in a scene view ...?
 *
 *
 */
class ScreenItem : public Item {

    friend class Item;
    friend class Layout;

protected: // constructor *********************************************************************************************/
    /** Default Constructor. */
    ScreenItem();

public: // methods ****************************************************************************************************/
    /** Destructor. */
    virtual ~ScreenItem() override;

    /** Returns the Item's applied transformation in parent space. */
    const Xform2f& get_transform() const { return m_applied_transform; }

    /** 2D transformation of this Item as determined by its parent Layout. */
    const Xform2f& get_layout_transform() const { return m_layout_transform; }

    /** 2D transformation of this Item on top of the layout transformation. */
    const Xform2f& get_local_transform() const { return m_local_transform; }

    /** Recursive implementation to produce the Item's transformation in window space. */
    Xform2f get_window_transform() const;

    /** Returns the unscaled size of this Item in pixels. */
    const Size2f& get_size() const { return m_size; }

    /** Returns the axis-aligned bounding rect of this ScreenItem in parent space. */
    Aabrf get_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem as transformed by its layout only. */
    Aabrf get_layout_aarbr() const;

    /** Returns the axis-aligned bounding rect of this ScreenItem in local space. */
    Aabrf get_local_aarbr() const;

    /** Returns the opacity of this Item in the range [0 -> 1].
     * @param own   By default, the returned opacity will be the product of this Item's opacity with all of its
     *              ancestors. If you set `own` to true, the opacity of this Item alone is returned.
     */
    float get_opacity(bool own = false) const;

    /** Sets the opacity of this Item.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     * @return          True if the opacity changed, false if the old value is the same as the new one.
     */
    bool set_opacity(float opacity);

    /** The current Claim of this Item. */
    const Claim& get_claim() const { return m_claim; }

    /** Checks, if the Item is currently visible.
     * This method does return false if the opacity is zero but also if there are any other factors that make this Item
     * not visible, like a zero size for example.
     */
    bool is_visible() const;

    /** Returns the Layout used to scissor this ScreenItem.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set, the scissor Layout has since expired or the
     * scissor Layout does not share a Window with this ScreenItem.
     * In this case, the ScreenItem is implicitly scissored by its parent Layout.
     */
    LayoutPtr get_scissor(const bool own = false) const; // TODO: propagate the scissor layout down, instead of querying it up

    /** Sets the new scissor Layout for this ScreenItem.
     * @param scissor               New scissor Layout, must be an Layout in this ScreenItem's ancestry or empty.
     * @throw std::runtime_error    If the scissor is not an ancestor Layout of this ScreenItem.
     */
    void set_scissor(LayoutPtr get_scissor);

    /** Updates the transformation of this Item.
     * @return      True iff the transform was modified.
     */
    bool set_local_transform(const Xform2f transform);

public: // signals ****************************************************************************************************/
    /** Emitted, when the opacity of this Item has changed.
     * @param New visiblity.
     */
    Signal<float> on_opacity_changed; // TODO: emit ScreenItem::opacity_changed also when a parent items' opacity changed?

    /** Emitted, when the size of this Item has changed.
     * @param New size.
     */
    Signal<const Size2f&> on_size_changed;

    /** Emitted, when the transform of this Item has changed.
     * @param New local transform.
     */
    Signal<const Xform2f&> on_transform_changed;

    /** Signal invoked when this Item is asked to handle a Mouse move event. */
    Signal<MouseEvent&> on_mouse_move;

    /** Signal invoked when this Item is asked to handle a Mouse button event. */
    Signal<MouseEvent&> on_mouse_button;

    /** Signal invoked when this Item is asked to handle a scroll event. */
    Signal<MouseEvent&> on_mouse_scroll;

    /** Signal invoked, when this Item is asked to handle a key event. */
    Signal<KeyEvent&> on_key;

    /** Signal invoked, when this Item is asked to handle a character input event. */
    Signal<CharEvent&> on_char_input;

    /** Emitted, when the Widget has gained or lost the Window's focus. */
    Signal<FocusEvent&> on_focus_changed;

protected: // methods *************************************************************************************************/
    /** Notifies the parent Layout that the Claim of this ScreenItem has changed.
     * The change propagates up the Item hierarchy until it reaches the first ancestor, that doesn't need to change its
     * Claim, where it proceeds downwards again to re-layout all changed Items.
     */
    void _update_parent_layout();

    /** Tells the Window that this Item needs to be redrawn.
     * @return False if the Widget is invisible and doesn't need to be redrawn.
     */
    bool _redraw();

    /** Updates the layout transformation of this Item.
     * @return      True iff the transform was modified.
     */
    bool _set_layout_transform(const Xform2f transform);

    /** Updates the size of this Item.
     * Note that the ScreenItem's Claim can impose hard constraints on the size, which are enforced by this method.
     * That means, you cannot assign a size to a ScreenItem that would violate its Claim.
     * Is virtual because Layouts use this function to update their Items.
     * @return      True iff the size was modified.
     */
    virtual bool _set_size(const Size2f size);

    /** Updates the Claim of this Item, may also change its size to comply with the new constraints.
     * @return      True iff the Claim was modified.
     */
    bool _set_claim(const Claim claim);

private: // methods ***************************************************************************************************/
    /** Calculates the transformation of this Item relative to its Window. */
    void _get_window_transform(Xform2f& result) const;

    /** Updates the ScreenItem's applied transform if either the layout- or local transform changed. */
    void _update_applied_transform();

private: // fields ****************************************************************************************************/
    /** Opacity of this Item in the range [0 -> 1]. */
    float m_opacity;

    /** Unscaled size of this Item in pixels. */
    Size2f m_size;

    /** 2D transformation of this Item as determined by its parent Layout. */
    Xform2f m_layout_transform;

    /** 2D transformation of this Item on top of the layout transformation. */
    Xform2f m_local_transform;

    /** Applied 2d transformation.
     * This value could be recalculated on-the-fly with `m_layout_transform * m_local_transform`, but usually it is
     * changed once and read many times which is why we store it.
     */
    Xform2f m_applied_transform;

    /** The Claim of a Item determines how much space it receives in the parent Layout.
     * Claim values are in untransformed local space.
     */
    Claim m_claim;

    /** Reference to a Layout used to 'scissor' this Widget.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;
};

/**********************************************************************************************************************/

/** Returns a transformation from a given ScreenItem to another one.
 * @param source    ScreenItem prodiving source coordinates in local space.
 * @param target    ScreenItem into which the coordinates should be transformed.
 * @return          Transformation.
 * @throw           std::runtime_error, if the two ScreenItems do not share a common ancestor.
 */
Xform2f get_transformation_between(const ScreenItem *source, const ScreenItem *target);

} // namespace notf
