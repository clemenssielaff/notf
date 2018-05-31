#pragma once

#include "app/node.hpp"
#include "app/widget/claim.hpp"
#include "common/aabr.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// A ScreenNode is the virtual base class for all objects in the Item hierarchy. Its three main specializations are
/// `Widgets`, `Layouts` and `Controllers`.
///
/// Lifetime
/// ========
///
/// The lifetime of Items is managed through a shared_ptr. This way, the user is free to keep a subhierarchy around
/// even after its parent has gone out of scope.
///
/// Hierarchy
/// =========
///
/// Items form a hierarchy with a single root Item on top. The number of children that an Item can have depends on its
/// type. Widgets have no children, Controller have a single Layout as a child and Layouts can have a (theoretically
/// unlimited) number of children of all types.
///
/// Since Layouts may have special container requirement for their children (a list, a map, a matrix ...), Items have a
/// virtual container class called `ChildContainer` that allows high level access to the children of each Item,
/// regardless of how they are stored in memory. The only requirements that a Container must fulfill is that it has to
/// offer a `size()` function that returns the number of children in the layout, a `clear()` function that removes all
/// children (thereby potentially destroying them) and the method `child_at(index)` which returns a mutable reference to
/// an pointer of the child at the requested index, or throws an `out_of_bounds` exception if the index is >= the
/// Container's size.
///
/// Nodes keep a raw pointer to their parent. The alternative would be to have a weak_ptr to the parent and lock
/// it, whenever we need to go up in the hierarchy. However, going up in the hierarchy is a very common occurrence and
/// with deeply nested layouts, I assume that the number of locking operations per second will likely go in the
/// thousands. This is a non-neglible expense for something that can prevented by making sure that you either know that
/// the parent is still alive, or you check first. The pointer is initialized to zero and parents let their children
/// know when they are destroyed. While this still leaves open the possiblity of an Item subclass manually messing up
/// the parent pointer using the static `_set_parent` method, I have to stop somewhere and trust the user not to break
/// things.
///
/// ID
/// ==
///
/// Each Node has a constant unique integer ID assigned to it upon instantiation. It can be used to identify the
/// node in a map, for debugging purposes or in conditionals.
///
/// Name
/// ====
///
/// In addition to the unique ID, each Node can have a name. The name is assigned by the user and is not guaranteed
/// to be unique. If the name is not set, it is custom to log the Node id instead, formatted like this:
///
///     log_info << "Something cool happened to Node #" << node.id() << ".";
///
/// Signals
/// =======
///
/// Nodes communicate with each other either through their relationship in the hierachy (parents to children and
/// vice-versa) or via Signals. Signals have the advantage of being able to connect every Node regardless of its
/// position in the hierarchy. They can be created by the user and enabled/disabled at will.
/// In order to facilitate Signal handling at the lowest possible level, all Nodes derive from the
/// `receive_signals` class that takes care of removing leftover connections that still exist once the Node goes
/// out of scope.
///

/// Abstract baseclass for all Node types in a Widget hierarchy that can occupy space on screen.
///
/// Layouting
/// =========
///
/// Layouts and Widgets need to "negotiate" the Layout of the application. NoTF's layout mechanism hinges on three
/// closely related concepts: Claims and Grants and Sizes.
///
/// Claim
/// -----
///
/// All ScreenItems have a Claim, that is a minimum / preferred / maximum 2D size, as well as a min/max ratio
/// constraint. The Claim lets the parent Layout know how much space the ScreenItem would like to occupy. The children
/// can be as greedy as they want, they don't care about how much space the parent actually owns. Claim coordinates are
/// in local (untransformed) space. The min/max sizes of the Claim are hard constraints, meaning that the ScreenItem
/// will never grow beyond its max or shrink below its min size.
///
/// Grant
/// -----
///
/// If the child ScreenItems claim more space than is available, the parent Layout will do its best to distribute
/// (grant) the space as fair as possible - but there is no way to guarantee that all ScreenItems will fit on screen at
/// once. Often, a Layout will receive a smaller Grant than it would require to accommodate all of its children. In that
/// case, it will take the Grant and calculate the smallest size that would work for all of its children, taking into
/// account the build-in behaviour of the Layout type. A wrapping FlexLayout, for example, will respect the horizontal
/// size of the Grant and only grow vertically, an Overlayout will adopt the size of the largest of its Children and a
/// FreeLayout will use the union of all of its children's bounding rects. The parts of the Layout's extend that are
/// beyond it's granted space will overflow. Depending on the scissoring behaviour, they might get cut off or simply
/// be drawn outside of the Layout's allocated space.
///
/// Think of the Grant as the extend that the parent expects its child to have, while its actual size is what the
/// ScreenItem decides for itself, based on its Claim.
///
/// Layout negotiation
/// ------------------
///
/// Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
/// If its Claim changes, its respective parent Layout might need to update as well - up to the first Layout that does
/// not update its Claim (at the latest, the RootLayout never updates its Claim).
///
/// The pipeline is as follows:
///
///      1. A ScreenItem changes its Claim. Either a Widget claims more/less space in response to an event, a Layout
///         finds itself with more/fewer children or whatever.
///      2. The ScreenItem notifies its parent Layout, which in turn updates its Claim and notifies its own parent.
///         This chain continues, until one Layout finds that its own Claim did not change after recalculation.
///      3. The first Layout with a non-changed Claim will re-layout all of its children and assign each one a new Grant
///         and transform. Layout children will react by themselves re-layouting and potentially resizing their own
///         children.
///
/// Content Aabr
/// ------------
///
/// Size is only the size of the Layout itself, how much was claimed and subsequently granted. The content Aabr is the
/// Aabr of all descendant nodes in the Layout. For example: a FlexLayout in a ScrollArea might have a (visible) size of
/// 600x400, but a content Aabr of 600x845 with 445 vertical rows being scissored by the ScrollArea.
///
/// Spaces
/// ------
///
/// *Untransformed space*
///
/// Claims are made in untransformed space.
/// That means, they are not affected by the offset transform applied to the ScreenItem, nor do they change when the
/// parent Layout changes the ScreenItem's layout transform.
/// The ScreenItem's size is in this space also.
///
/// *Offset space*
///
/// Each ScreenItem has full control over its own offset.
/// The offset is applied first and does not influence how the Layout perceives the ScreenItem, meaning if you scale the
/// ScreenItem twofold, it will appear bigger on screen but the scale will remain invisible to the the parent Layout.
/// That also means that clicking the cursor into the overflow areas will not count as a click inside the ScreenItem,
/// because the parent won't know that it appears bigger on screen.
/// Offsets are useful, for example, to apply a jitter animation or similar transformations that should not affect the
/// layout.
///
/// *Layout (parent) space*
///
/// Transformation controlled by the parent Layout. Used to position the ScreenItem within the parent Layout.
///
/// *Window space*
///
/// Transformation relative to the RootLayout. The RootLayout might itself have a transformation, but this should be
/// transparent to the nodes in its hierarchy and if the root's Layer is in fullscreen mode, Window space is accurate.
///
/// Opacity
/// -------
///
/// Each ScreenItem has an `opacity` member, which is a float in the range [0 -> 1]. An opacity of `0` means that the
/// node is fully transparent (invisible, in fact), an opacity of `0.5` (semi-transparent) and `1` not transparent at
/// all. Opacity trickles down the hierarchy, meaning that in order to get to the effective opacity of a ScreenItem, you
/// have to multiply it's own opacity with that of each ancestor.
///
/// Scissoring
/// ----------
///
/// In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
/// those Widgets need to be "scissored" by the scroll area. A "scissor" is an axis-aligned rectangle, scissoring is the
/// act of cutting off parts of a Widget that fall outside that rectangle. Every Widget contains a pointer to the
/// ancestor Layout that acts as its scissor. By default, all ScreenItems are scissored to the RootLayout, but you can
/// explicitly override the scissor Layout for each ScreenItem individually. If a ScreenItem is moved outside of its
/// scissor hierarchy, it will fall back to its parent's scissor Layout.
///
/// Events
/// ======
///
/// All ScreenItems can handle events. Events are created by the Application in reaction to something happening, like a
/// user input or a system event. Only Widgets receive events, which means that you might want to put an invisible
/// Widget in the background of a Layout to catch events that would otherwise "fall through the cracks" (see ScrollArea
/// for an example). If a Widget receives an event but does not handle it, the eventis propagated up the ancestry until
/// it either reaches the root node or an ancestor sets the event's `is_handled` flag.
///
class ScreenItem : public Node {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    NOTF_ALLOW_ACCESS_TYPES(RootLayout);

    /// Spaces that the transformation of a ScreenItem passes through.
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

    /// Emitted when the scissor of this ScreenItem changed.
    /// @param New scissor Layout
    Signal<const Layout*> on_scissor_changed;

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
    /// @param token        Factory token provided by Node::_create.
    /// @param container    Container used to store this Item's children.
    ScreenItem(const Token& token, ChildContainerPtr container) : Node(token, std::move(container)) {}

public:
    /// Destructor.
    virtual ~ScreenItem() override = default;

    /// The Claim of this ScreenItem.
    const Claim& claim() const { return m_claim; }

    /// ScreenItem's transformation in the requested space.
    template<Space space>
    const Matrix3f xform() const
    {
        static_assert(always_false<Space, space>{}, "Unsupported Space for ScreenItem::xform");
    }

    /// Axis-aligned bounding rect around all children of this ScreenItem in local space.
    const Aabrf& content_aabr() const { return m_content_aabr; }

    /// The space allowance granted by the parent Layout.
    const Size2f& grant() const { return m_grant; }

    /// Unscaled actual size of this ScreenItem in local space.
    const Size2f& size() const { return m_size; }

    /// The axis-aligned bounding rect of this ScreenItem in the requested space.
    template<Space space = Space::LOCAL>
    Aabrf aabr() const
    {
        return xform<space>().transform(Aabrf(m_size));
    }

    /// Returns the Layout used to scissor this ScreenItem or `nullptr`, if the ScreenItem uses the nearest scissor
    /// Layout defined in its hierarchy.
    const Layout* scissor() const { return m_scissor_layout; }

    /// Returns the effective opacity of this ScreenItem in the range [0 -> 1].
    /// @param effective    By default, the returned opacity will be the product of this ScreenItem's opacity with all
    ///                     of its ancestors'. If set to false, the opacity of this ScreenItem alone is returned.
    float opacity(bool effective = true) const;

    /// Checks, if the ScreenItem is currently visible.
    /// This method does return false if the opacity is zero but also if there are any other factors that make this
    /// ScreenItem not visible, like a zero size for example or being completely scissored.
    bool is_visible() const;

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    virtual void widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const = 0;

    /// Updates the offset transformation of this ScreenItem.
    void set_offset_xform(const Matrix3f transform);

    /// Sets the opacity of this ScreenItem.
    /// @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
    void set_opacity(float opacity);

    /// Sets the visibility flag of this ScreenItem.
    /// Note that the ScreenItem is not guaranteed to be visible just because the visibility flag is true (see
    /// `is_visible` for details).
    /// If the flag is false however, the ScreenItem is guaranteed to be not visible.
    void set_visible(bool is_visible);

    /// Sets the new scissor Layout for this ScreenItem.
    void set_scissor(const Layout* scissor_layout);

protected:
    void _redraw() const {} // TODO: remove

    /// Updates the size of this ScreenItem and the layout of all child Items.
    virtual void _relayout() = 0;

    /// Queries new data from the parent (what that is depends on the ScreenItem type).
    virtual void _update_from_parent() override;

    /// Lets the parent Layout know that this ScreenItem has changed at it might have to relayout.
    void _update_parent_layout();

    /// Updates the Claim of this Item, which might cause a relayout of itself and its ancestor Layouts.
    /// We do not allow direct access to the Claim because modifying it without updating the ScreenItem hierarchy
    /// has no effect and could cause confusion.
    /// @param claim    New Claim.
    /// @return         True iff the Claim was modified.
    bool _set_claim(const Claim claim);

    /// Updates the layout transformation of this ScreenItem.
    /// @param transform    New Layout transform.
    void _set_layout_xform(const Matrix3f transform);

    /// Updates the ScreenItem's content Aabr.
    /// @param aabr  New content Aabr.
    void _set_content_aabr(const Aabrf aabr);

    /// Sets a new Scissor Layout for this ScreenItem.
    /// @param scissor_layout   New scissor Layout.
    void _set_scissor(const Layout* scissor_layout);

    /// Updates the Grant of this ScreenItem and might cause a relayout.
    /// @param grant    New Grant.
    /// @return         True iff the Grant was modified.
    bool _set_grant(const Size2f grant);

    /// Updates the size of this ScreenItem.
    /// @param size     New size.
    /// @return         True iff the size  was modified.
    bool _set_size(const Size2f size);

    /// Allows Layouts to assign grants to other ScreenItems.
    /// @return      True iff the ScreenItem's Grant was modified.
    static bool _set_grant(ScreenItem* screen_item, Size2f grant) { return screen_item->_set_grant(std::move(grant)); }

    /// Allows ScreenItem subclasses to change each other's layout transformation.
    /// @param transform    New Layout transform.
    static void _set_layout_xform(ScreenItem* screen_item, Matrix3f xform)
    {
        return screen_item->_set_layout_xform(std::move(xform));
    }

private:
    /// Calculates the transformation of this ScreenItem relative to its Window.
    void _window_transform(Matrix3f& result) const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The Claim of a ScreenItem determines how much space it receives in the parent Layout.
    /// Claim values are in untransformed local space.
    Claim m_claim;

    /// 2D transformation of this ScreenItem as determined by its parent Layout.
    Matrix3f m_layout_transform = Matrix3f::identity();

    /// 2D transformation of this ScreenItem on top of the layout transformation.
    Matrix3f m_offset_transform = Matrix3f::identity();

    /// The bounding rect of all descendant ScreenItems.
    Aabrf m_content_aabr = Aabrf::zero();

    /// Reference to a Layout in the ancestry, used to 'scissor' this ScreenItem.
    const Layout* m_scissor_layout = nullptr;

    /// The grant of a ScreenItem is how much space is 'granted' to it by its parent Layout.
    /// Depending on the parent Layout, the ScreenItem's Claim can be used to influence the grant.
    /// Note that the grant can also be smaller or bigger than the Claim.
    Size2f m_grant = Size2f::zero();

    /// The size of a ScreenItem is how much space the ScreenItem claims after receiving a grant from its parent Layout.
    Size2f m_size = Size2f::zero();

    /// Opacity of this ScreenItem in the range [0 -> 1].
    float m_opacity = 1;

    /// Flag indicating whether a ScreenItem should be visible or not.
    /// Note that the ScreenItem is not guaranteed to be visible just because this flag is true.
    /// If the flag is false however, the ScreenItem is guaranteed to be invisible.
    bool m_is_visible = true;
};

// ================================================================================================================== //

template<>
inline const Matrix3f ScreenItem::xform<ScreenItem::Space::LOCAL>() const
{
    return Matrix3f::identity();
}

template<>
inline const Matrix3f ScreenItem::xform<ScreenItem::Space::OFFSET>() const
{
    return m_offset_transform;
}

template<>
inline const Matrix3f ScreenItem::xform<ScreenItem::Space::LAYOUT>() const
{
    return m_layout_transform;
}

template<>
inline const Matrix3f ScreenItem::xform<ScreenItem::Space::PARENT>() const
{
    return m_offset_transform * m_layout_transform;
}

template<>
const Matrix3f ScreenItem::xform<ScreenItem::Space::WINDOW>() const;

// ================================================================================================================== //

template<>
class ScreenItem::Access<RootLayout> {
    friend class RootLayout;

    /// Constructor.
    /// @param screen_item  ScreenItem to access.
    Access(ScreenItem& screen_item) : m_screen_item(screen_item) {}

    /// Turns this ScreenItem into a root ScreenItem that is its own scissor.
    void be_own_scissor(Layout* root_layout) { m_screen_item.m_scissor_layout = root_layout; }

    /// The ScreenItem to access.
    ScreenItem& m_screen_item;
};

// ================================================================================================================== //

///@{
/// Returns the ScreenItem associated with thr given Item - either the Item itself or a Controller's root Item.
/// Is empty if this is a Controller without a root Item.
/// @param item     Item to query.
risky_ptr<ScreenItem> get_screen_item(Item* item);
inline risky_ptr<const ScreenItem> get_screen_item(const Item* item)
{
    return get_screen_item(const_cast<Item*>(item));
}
inline risky_ptr<ScreenItem> get_screen_item(risky_ptr<Item> item)
{
    if (item) {
        return get_screen_item(&*item);
    }
    else {
        return {};
    }
}
inline risky_ptr<const ScreenItem> get_screen_item(risky_ptr<const Item> item)
{
    if (item) {
        return get_screen_item(&*item);
    }
    else {
        return {};
    }
}
///@}

/// Calculates a transformation from a given ScreenItem to another one.
/// @param source    ScreenItem providing source coordinates in local space.
/// @param target    ScreenItem into which the coordinates should be transformed.
/// @return          Transformation.
/// @throw           std::runtime_error, if the two ScreenItems do not share a common ancestor.
Matrix3f transformation_between(const ScreenItem* source, const ScreenItem* target);

NOTF_CLOSE_NAMESPACE
