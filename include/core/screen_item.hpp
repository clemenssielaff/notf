#pragma once

#include "core/item.hpp"

namespace notf {

/**
 * Baseclass for all Item%s that have physical expansion.
 */
class ScreenItem : public Item {

    friend class Item;
    friend class Layout;

protected: // constructor
    /** Default Constructor. */
    explicit ScreenItem();

public: // methods
    virtual ~ScreenItem() override;

    /** Returns the Item's transformation in parent space. */
    const Xform2f& get_transform() const { return m_transform; }

    /** Recursive implementation to produce the Item's transformation in window space. */
    Xform2f get_window_transform() const;

    /** Returns the unscaled size of this Item in pixels. */
    const Size2f& get_size() const { return m_size; }

    /** Returns the opacity of this Item in the range [0 -> 1]. */
    float get_opacity() const { return m_opacity; }

    /** Sets the opacity of this Item.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     * @return          True if the opacity changed, false if the old value is the same as the new one.
     */
    bool set_opacity(float get_opacity);

    /** The current Claim of this Item. */
    const Claim& get_claim() const { return m_claim; }

    /** Checks, if the Item is currently visible.
     * This method does return false if the opacity is zero but also if there are any other factors that make this Item
     * not visible, like a zero size for example.
     */
    bool is_visible() const
    {
        return !(false
                 || get_size().is_zero()
                 || !get_size().is_valid()
                 || get_opacity() <= precision_high<float>());
    }

public: // signals
    /** Emitted, when the opacity of this Item has changed.
     * @param New visiblity.
     */
    Signal<float> opacity_changed;

    /** Emitted, when the size of this Item has changed.
     * @param New size.
     */
    Signal<Size2f> size_changed;

    /** Emitted, when the transform of this Item has changed.
     * @param New local transform.
     */
    Signal<Xform2f> transform_changed;

protected: // methods
    /** Tells the Window that this Item needs to be redrawn.
     * @return False if the Widget is invisible and doesn't need to be redrawn.
     */
    bool _redraw();

    /** Updates the size of this Item.
     * Is virtual because Layouts can use this function to update their items.
     * @return      True iff the size has been modified.
     */
    virtual bool _set_size(const Size2f& get_size);

    /** Updates the transformation of this Item.
     * @return      True iff the transform has been modified.
     */
    bool _set_transform(const Xform2f get_transform);

    /** Updates the Claim but does not trigger any layouting.
     * @return      True iff the Claim was changed.
     */
    bool _set_claim(const Claim get_claim);

private: // methods
    /** Calculates the transformation of this Item relative to its Window. */
    void _get_window_transform(Xform2f& result) const;

private: // fields
    /** Opacity of this Item in the range [0 -> 1]. */
    float m_opacity;

    /** Unscaled size of this Item in pixels. */
    Size2f m_size;

    /** 2D transformation of this Item in local space. */
    Xform2f m_transform;

    /** The Claim of a Item determines how much space it receives in the parent Layout. */
    Claim m_claim;
};

} // namespace notf
