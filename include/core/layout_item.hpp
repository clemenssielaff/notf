#pragma once

#include "core/item.hpp"

namespace notf {

/**
 * Baseclass for all Item%s that have physical expansion.
 */
class LayoutItem : public Item {

public: // methods
    virtual ~LayoutItem() override;

    virtual float get_opacity() const override { return m_opacity; }

    virtual const Size2f& get_size() const override { return m_size; }

    virtual const Claim& get_claim() const override { return m_claim; }

    /** Sets the opacity of this LayoutItem.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     */
    void set_opacity(const float opacity) { _set_opacity(opacity); }

protected: // methods
    explicit LayoutItem()
        : Item()
        , m_opacity(1)
        , m_size()
        , m_transform(Transform2::identity())
        , m_claim()
    {
    }

    /** Sets the opacity of this Item. */
    virtual bool _set_opacity(float opacity) override
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

    /** Updates the size of this Item.
     * Is virtual, since Layouts use this function to update their items.
     * @return      True iff the size has been modified.
     */
    virtual bool _set_size(const Size2f size) override
    {
        if (size != m_size) {
            m_size = std::move(size);
            size_changed(m_size);
            _redraw();
            return true;
        }
        return false;
    }

    /** Updates the transformation of this Item. */
    virtual bool _set_transform(const Transform2 transform) override
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
    virtual Transform2 _get_local_transform() const override { return m_transform; }

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
