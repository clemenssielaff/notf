#pragma once

#include "app/node.hpp"
#include "app/widget/claim.hpp"
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
    Widget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent, const std::string& name = {})
        : Node(token, scene, parent)
    {
        // name the widget
        if (!name.empty()) {
            set_name(name);
        }

        // create widget properties
        m_claim = _create_property<Claim>("claim", Claim(), {}, /* has_body = */ false);
        m_layout_transform = _create_property<Matrix3f>("layout_transform", Matrix3f::identity());
        m_offset_transform = _create_property<Matrix3f>("offset_transform", Matrix3f::identity());
        m_content_aabr = _create_property<Aabrf>("content_aabr", Aabrf::zero(), {}, /* has_body = */ false);
        m_grant = _create_property<Size2f>("grant", Size2f::zero(), {}, /* has_body = */ false);
        m_opacity = _create_property<float>("opacity", 1, [](float& v) {
            v = clamp(v, 0, 1);
            return true;
        });
        m_visibility = _create_property<bool>("visibility", true);
    }

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

    /// The axis-aligned bounding rect around this and all children of this Widget in the requested space.
    template<Space space = Space::LOCAL>
    Aabrf get_aabr() const
    {
        return get_xform<space>().transform(m_content_aabr.get());
    }

protected:
    virtual void _paint() = 0;

    /// Updates the size of this and the position/size of all child Widgets.
    void _relayout();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
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

// ================================================================================================================== //

/// Calculates a transformation from a given Widget to another one.
/// @param source    Widget providing source coordinates in local space.
/// @param target    Widget into which the coordinates should be transformed.
/// @return          Transformation.
/// @throw           std::runtime_error, if the two Widgets do not share a common ancestor.
Matrix3f transformation_between(const Widget* source, const Widget* target);

NOTF_CLOSE_NAMESPACE
