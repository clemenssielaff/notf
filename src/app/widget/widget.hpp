#pragma once

#include "app/node.hpp"
#include "app/widget/claim.hpp"
#include "app/widget/clipping.hpp"
#include "app/widget/design.hpp"
#include "common/aabr.hpp"
#include "common/matrix3.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Widget : public Node {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Spaces that the transformation of a Widget passes through.
    enum class Space : uchar {
        LOCAL,  // no transformation
        OFFSET, // offset transformation only
        LAYOUT, // layout transformation only
        PARENT, // offset then layout transformation
        WINDOW, // transformation relative to the RootLayout
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    Widget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent, const std::string& name = {});

    /// Destructor.
    ~Widget() override;

    /// The Claim of this Widget.
    const Claim& get_claim() const { return m_claim.get(); }

    /// Widget's transformation in the requested space.
    template<Space space>
    const Matrix3f get_xform() const
    {
        static_assert(always_false<Space, space>{}, "Unsupported Space for Widget::get_xform");
    }

    /// Calculates a transformation from this to another Widget.
    /// @param target    Widget into which to transform.
    /// @throw           hierarchy_error if the two Widgets are not in the same Scene.
    Matrix3f get_xform_to(valid_ptr<const Widget*> target) const;

    /// The axis-aligned bounding rect around this and all children of this Widget in the requested space.
    template<Space space = Space::LOCAL>
    Aabrf get_aabr() const
    {
        return get_xform<space>().transform(m_content_aabr.get());
    }

    /// The Clipping rect of this Widget.
    virtual const Clipping& get_clipping_rect() const;

protected:
    /// Updates the Design of this Widget through the given Painter.
    virtual void _paint(Painter& painter) = 0;

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<valid_ptr<Widget*>>& result) const = 0;

    /// Allows Widget subclasses to call `_get_widget_at` on each other.
    static void
    _get_widgets_at(const valid_ptr<Widget*> widget, const Vector2f& local_pos, std::vector<valid_ptr<Widget*>>& result)
    {
        widget->_get_widgets_at(local_pos, result);
    }

private:
    /// Changing the Claim or the visiblity of a Widget causes a relayout further up the hierarchy.
    void _relayout_upwards();

    /// Updates the size and transformations of all child Widgets.
    virtual void _relayout_downwards() {}

    /// Recalcuates the Claim of this Widget.
    /// Useful for example, when the Widget's Claim is determined by its children.
    virtual Claim _update_claim() { return m_claim.get(); }

    /// Updates (if necessary) and returns the Design of this Widget.
    const WidgetDesign& get_design();

    /// Calculates the transformation of this Widget relative to its Window.
    void _get_window_xform(Matrix3f& result) const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Design of this Widget.
    WidgetDesign m_design;

    /// The Claim of a Widget determines how much space it receives in the parent Layout.
    /// Claim values are in untransformed local space.
    PropertyHandle<Claim> m_claim;

    /// 2D transformation of this Widget as determined by its parent Layout.
    PropertyHandle<Matrix3f> m_layout_transform;

    /// 2D transformation of this Widget on top of the layout transformation.
    PropertyHandle<Matrix3f> m_offset_transform;

    /// The bounding rect of all descendant Widgets.
    PropertyHandle<Aabrf> m_content_aabr;

    /// The grant of a Widget is how much space is 'granted' to it by its parent Layout.
    /// Depending on the parent Layout, the Widget's Claim can be used to influence the grant.
    /// Note that the grant can also be smaller or bigger than the Claim.
    PropertyHandle<Size2f> m_grant;

    /// Opacity of this Widget in the range [0 -> 1].
    PropertyHandle<float> m_opacity;

    /// Flag indicating whether this Widget should be visible or not.
    /// Note that the Widget is not guaranteed to be visible just because this flag is true.
    /// If the flag is false however, the Widget is guaranteed to be invisible.
    PropertyHandle<bool> m_visibility;
};

template<>
inline const Matrix3f Widget::get_xform<Widget::Space::LOCAL>() const
{
    return Matrix3f::identity();
}

template<>
inline const Matrix3f Widget::get_xform<Widget::Space::OFFSET>() const
{
    return m_offset_transform.get();
}

template<>
inline const Matrix3f Widget::get_xform<Widget::Space::LAYOUT>() const
{
    return m_layout_transform.get();
}

template<>
inline const Matrix3f Widget::get_xform<Widget::Space::PARENT>() const
{
    return m_offset_transform.get() * m_layout_transform.get();
}

template<>
const Matrix3f Widget::get_xform<Widget::Space::WINDOW>() const;

NOTF_CLOSE_NAMESPACE
