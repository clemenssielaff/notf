#pragma once

#include "core/item.hpp"

namespace notf {

/** Baseclass for all Items that have physical expansion (Widgets and Layouts).
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
 * Scissoring
 * ==========
 * In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
 * those Widgets need to be "scissored" by the scroll area.
 * A "Scissor" is an axis-aligned rectangle, scissoring is the act of cutting off parts of a Widget that fall outside
 * that rectangle and since this operation is provided by OpenGL, it is pretty cheap.
 * Every Widget contains a pointer to the parent Layout that acts as its scissor.
 * An empty pointer means that this Widget is scissored by its parent Layout, but every Layout in this Widget's Item
 * ancestry can be used (including the Windows WindowLayout, which effectively disables scissoring).
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

    /** Returns the Item's transformation in parent space. */
    const Xform2f& get_transform() const { return m_transform; }

    /** Recursive implementation to produce the Item's transformation in window space. */
    Xform2f get_window_transform() const;

    /** Returns the unscaled size of this Item in pixels. */
    const Size2f& get_size() const { return m_size; }

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
    LayoutPtr get_scissor(const bool own = false) const;

    /** Sets the new scissor Layout for this ScreenItem.
     * @param scissor               New scissor Layout, must be an Layout in this ScreenItem's ancestry or empty.
     * @throw std::runtime_error    If the scissor is not an ancestor Layout of this ScreenItem.
     */
    void set_scissor(LayoutPtr get_scissor);

public: // signals ****************************************************************************************************/
    /** Emitted, when the opacity of this Item has changed.
     * @param New visiblity.
     */
    Signal<float> opacity_changed; // TODO: emit ScreenItem::opacity_changed also when a parent items' opacity changed?

    /** Emitted, when the size of this Item has changed.
     * @param New size.
     */
    Signal<Size2f> size_changed;

    /** Emitted, when the transform of this Item has changed.
     * @param New local transform.
     */
    Signal<Xform2f> transform_changed;

protected: // methods *************************************************************************************************/
    /** Tells the Window that this Item needs to be redrawn.
     * @return False if the Widget is invisible and doesn't need to be redrawn.
     */
    bool _redraw();

    /** Updates the size of this Item.
     * Note that the ScreenItem's Claim can impose hard constraints on the size, which are enforced by this method.
     * That means, you cannot assign a size to a ScreenItem that would violate its Claim.
     * Is virtual because Layouts use this function to update their Items.
     * @return      True iff the size was modified.
     */
    virtual bool _set_size(const Size2f size);

    /** Updates the transformation of this Item.
     * @return      True iff the transform was modified.
     */
    bool _set_transform(const Xform2f transform);

    /** Updates the Claim of this Item, may also change its size to comply with the new constraints.
     * @return      True iff the Claim was modified.
     */
    bool _set_claim(const Claim claim);

private: // methods ***************************************************************************************************/
    /** Calculates the transformation of this Item relative to its Window. */
    void _get_window_transform(Xform2f& result) const;

private: // fields ****************************************************************************************************/
    /** Opacity of this Item in the range [0 -> 1]. */
    float m_opacity;

    /** Unscaled size of this Item in pixels. */
    Size2f m_size;

    /** 2D transformation of this Item in relation to its parent. */
    Xform2f m_transform;

    /** The Claim of a Item determines how much space it receives in the parent Layout. */
    Claim m_claim;

    /** Reference to a Layout used to 'scissor' this Widget.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;
};

} // namespace notf
