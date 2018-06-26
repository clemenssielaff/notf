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

    // signals ------------------------------------------------------------------------------------------------------ //
public:
    /// Emitted, when the size of this ScreenItem has changed.
    /// @param New size.
    Signal<const Size2f&> on_size_changed;

    /// Emitted, when the transform of this ScreenItem has changed.
    /// @param New transform in parent space.
    Signal<const Matrix3f&> on_xform_changed;

    /// Emitted when the visibility flag was changed by the user.
    /// See `set_visible()` for details.
    /// @param Whether the ScreenItem is visible or not.
    Signal<bool> on_visibility_changed;

    /// Emitted, when the opacity of this ScreenItem has changed.
    /// Note that the effective opacity of a ScreenItem is determined through the multiplication of all of its
    /// ancestors opacity. If an ancestor changes its opacity, only itself will fire this signal.
    /// @param New visibility.
    Signal<float> on_opacity_changed;

    /// Signal invoked when this ScreenItem is asked to handle a Mouse move event.
    /// @param Mouse event.
    Signal<MouseEvent&> on_mouse_move;

    /// Signal invoked when this ScreenItem is asked to handle a Mouse button event.
    /// @param Mouse event.
    Signal<MouseEvent&> on_mouse_button;

    /// Signal invoked when this ScreenItem is asked to handle a scroll event.
    /// @param Mouse event.
    Signal<MouseEvent&> on_mouse_scroll;

    /// Signal invoked, when this ScreenItem is asked to handle a key event.
    /// @param Key event.
    Signal<KeyEvent&> on_key;

    /// Signal invoked, when this ScreenItem is asked to handle a character input event.
    /// @param Char event.
    Signal<CharEvent&> on_char_input;

    /// Signal invoked when this ScreenItem is asked to handle a WindowEvent.
    /// @param Window event.
    Signal<WindowEvent&> on_window_event;

    /// Emitted, when the ScreenItem has gained or lost the Window's focus.
    /// @param Focus event.
    Signal<FocusEvent&> on_focus_changed;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
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
        m_clipper = _create_property<risky_ptr<const Widget*>>("clipper", nullptr, {}, /* has_body = */ false);
        m_grant = _create_property<Size2f>("grant", Size2f::zero(), {}, /* has_body = */ false);
        m_size = _create_property<Size2f>("size", Size2f::zero(), {}, /* has_body = */ false);
        m_opacity = _create_property<float>("opacity", 1, [](float& v) {
            v = clamp(v, 0, 1);
            return true;
        });
        m_visibility = _create_property<bool>("visibility", true);
    }

    /// Destructor.
    ~Widget() override;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The Claim of a ScreenItem determines how much space it receives in the parent Layout.
    /// Claim values are in untransformed local space.
    PropertyHandle<Claim> m_claim;

    /// 2D transformation of this ScreenItem as determined by its parent Layout.
    PropertyHandle<Matrix3f> m_layout_transform;

    /// 2D transformation of this ScreenItem on top of the layout transformation.
    PropertyHandle<Matrix3f> m_offset_transform;

    /// The bounding rect of all descendant ScreenItems.
    PropertyHandle<Aabrf> m_content_aabr;

    /// Reference to a Layout in the ancestry, used to 'clip' this ScreenItem.
    PropertyHandle<risky_ptr<const Widget*>> m_clipper;

    /// The grant of a ScreenItem is how much space is 'granted' to it by its parent Layout.
    /// Depending on the parent Layout, the ScreenItem's Claim can be used to influence the grant.
    /// Note that the grant can also be smaller or bigger than the Claim.
    PropertyHandle<Size2f> m_grant;

    /// The size of a Widget is how much space the Widget claims after receiving the grant from its parent's Layout.
    PropertyHandle<Size2f> m_size; // TODO: what is this?

    /// Opacity of this Widget in the range [0 -> 1].
    PropertyHandle<float> m_opacity;

    /// Flag indicating whether this Widget should be visible or not.
    /// Note that the Widget is not guaranteed to be visible just because this flag is true.
    /// If the flag is false however, the Widget is guaranteed to be invisible.
    PropertyHandle<bool> m_visibility;
};

NOTF_CLOSE_NAMESPACE
