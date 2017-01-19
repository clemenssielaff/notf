#pragma once

#include "core/item.hpp"

namespace notf {

/**
 * Baseclass for all Item%s that have physical expansion.
 */
class LayoutItem : public Item {

    friend class Item;

public: // methods
    virtual ~LayoutItem() override;

    virtual LayoutItem* get_layout_item() override { return this; }

    virtual const LayoutItem* get_layout_item() const override { return this; }

    /** Returns the opacity of this Item in the range [0 -> 1]. */
    float get_opacity() const { return m_opacity; }

    /** Returns the unscaled size of this Item in pixels. */
    const Size2f& get_size() const { return m_size; }

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
                 || get_opacity() == approx(0));
    }

    /** Sets the opacity of this LayoutItem.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     * @return          True if the opacity changed, false if the old value is the same as the new one.
     */
    bool set_opacity(float opacity)
    {
        opacity = clamp(opacity, 0, 1);
        if (opacity != approx(m_opacity)) {
            m_opacity = opacity;
            opacity_changed(m_opacity);
            _redraw();
            return true;
        }
        return false;
    }

    /** Recursive implementation to produce the Item's transformation in window space. */
    Transform2 get_window_transform() const
    {
        Transform2 result = Transform2::identity();
        _get_window_transform(result);
        return result;
    }

    /** Returns the Item's transformation in parent space. */
    const Transform2& get_transform() const { return m_transform; }

    /** Looks for all Widgets at a given position in parent space.
     * @param local_pos     Local coordinates where to look for a Widget.
     * @param result        [out] All Widgets at the given coordinate, orderd from front to back.
     * @return              True if any Widget was found, false otherwise.
     */
    virtual bool get_widgets_at(const Vector2 local_pos, std::vector<Widget*>& result) = 0;

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
    Signal<Transform2> transform_changed;

protected: // methods
    explicit LayoutItem()
        : Item()
        , m_opacity(1)
        , m_size()
        , m_transform(Transform2::identity())
        , m_claim()
    {
    }

    /** Tells the Window that its contents need to be redrawn. */
    bool _redraw();

    /** Updates the size of this Item.
     * Is virtual because Layouts can use this function to update their items.
     * @return      True iff the size has been modified.
     */
    virtual bool _set_size(const Size2f& size)
    {
        if (size != m_size) {
            const Claim::Stretch& horizontal = m_claim.get_horizontal();
            const Claim::Stretch& vertical   = m_claim.get_vertical();
            // TODO: enforce width-to-height constraint in LayoutItem::_set_size (the StackLayout does something like it already)

            m_size.width  = max(horizontal.get_min(), min(horizontal.get_max(), size.width));
            m_size.height = max(vertical.get_min(), min(vertical.get_max(), size.height));
            size_changed(m_size);
            _redraw();
            return true;
        }
        return false;
    }

    /** Updates the transformation of this LayoutItem.
     * @return      True iff the transform has been modified.
     */
    bool _set_transform(const Transform2 transform)
    {
        if (transform != m_transform) {
            m_transform = std::move(transform);
            transform_changed(m_transform);
            _redraw();
            return true;
        }
        return false;
    }

    /** Updates the Claim but does not trigger any layouting.
     * @return True iff the Claim was changed.
     */
    bool _set_claim(const Claim claim)
    {
        if (claim != m_claim) {
            m_claim = std::move(claim);
            return true;
        }
        return false;
    }

private: // methods
    /** Calculates the transformation of this LayoutItem relative to its Window. */
    void _get_window_transform(Transform2& result) const;

private: // fields
    /** Opacity of this Item in the range [0 -> 1]. */
    float m_opacity;

    /** Unscaled size of this Item in pixels. */
    Size2f m_size;

    /** 2D transformation of this Item in local space. */
    Transform2 m_transform;

    /** The Claim of a LayoutItem determines how much space it receives in the parent Layout. */
    Claim m_claim;
};

} // namespace notf
