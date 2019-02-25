#pragma once

#include "notf/meta/pointer.hpp"

#include "notf/graphic/plotter/design.hpp"
#include "notf/graphic/plotter/painter.hpp"

#include "notf/app/graph/node.hpp"
#include "notf/app/widget/widget_claim.hpp"

NOTF_OPEN_NAMESPACE

// widget policy ==================================================================================================== //

namespace detail {
namespace widget_policy {

/// 2D transformation of this Widget on top of the layout transformation.
struct OffsetXform {
    using value_t = M3f;
    static constexpr ConstString name = "offset_xform";
    static inline const value_t default_value = M3f::identity();
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REDRAW;
};

/// Opacity of this Widget in the range [0 -> 1].
struct Opacity {
    using value_t = float;
    static constexpr ConstString name = "opacity";
    static inline const value_t default_value = 1.f;
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REDRAW;
};

/// Flag indicating whether this Widget should be visible or not.
/// Note that the Widget is not guaranteed to be visible just because this flag is true.
/// If the flag is false however, the Widget is guaranteed to be invisible.
struct Visibility {
    using value_t = bool;
    static constexpr ConstString name = "visibility";
    static inline const value_t default_value = true;
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REDRAW;
};

// slots ======================================================================

// struct CloseSlot {
//    using value_t = None;
//    static constexpr ConstString name = "to_close";
//};

// signals ====================================================================

// struct AboutToCloseSignal {
//    using value_t = None;
//    static constexpr ConstString name = "on_about_to_close";
//};

// policy ===================================================================

struct WidgetPolicy {
    using properties = std::tuple< //
        OffsetXform,               //
        Opacity,                   //
        Visibility                 //
        >;                         //

    using slots = std::tuple< //
        >;                    //

    using signals = std::tuple< //
        >;                      //
};

} // namespace widget_policy
} // namespace detail

// any widget ======================================================================================================= //

class AnyWidget : public Node<detail::widget_policy::WidgetPolicy> {

    friend WidgetHandle;

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class type.
    using super_t = Node<detail::widget_policy::WidgetPolicy>;

    /// Internal reactive function that is subscribed to all REFRESH Properties and clears the WidgetDesign, should one
    /// of them change.
    class RefreshObserver : public Subscriber<All> {

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param widget   Widget owning this Subscriber.
        RefreshObserver(AnyWidget& widget) : m_widget(widget) {}

        /// Called whenever a REFRESH Property changed its value.
        void on_next(const AnyPublisher*, ...) final { m_widget.m_design.reset(); }

        // fields ---------------------------------------------------------- //
    private:
        /// Widget owning this Subscriber.
        AnyWidget& m_widget;
    };
    using RefreshObserverPtr = std::shared_ptr<RefreshObserver>;

public:
    /// Spaces that the transformation of a Widget passes through.
    enum class Space : uchar {
        LOCAL,  // no transformation
        OFFSET, // offset transformation only
        LAYOUT, // layout transformation only
        PARENT, // offset then layout transformation
        WINDOW, // transformation relative to the RootLayout
    };

    /// Error thrown when a requested State transition is impossible.
    NOTF_EXCEPTION_TYPE(BadTransitionError);

    /// Property names.
    static constexpr const ConstString& offset_xform = detail::widget_policy::OffsetXform::name;
    static constexpr const ConstString& opacity = detail::widget_policy::Opacity::name;
    static constexpr const ConstString& visibility = detail::widget_policy::Visibility::name;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    AnyWidget(valid_ptr<AnyNode*> parent);

    /// Virtual destructor.
    virtual ~AnyWidget() = default;

    // layout -----------------------------------------------------------------

    /// Widget's transformation in the requested space.
    template<Space space>
    M3f get_xform() const;

    /// The axis-aligned bounding rect around this and all children of this Widget in the requested space.
    template<Space space = Space::LOCAL>
    Aabrf get_aabr() const {
        return transform_by(m_content_aabr, get_xform<space>());
    }

    /// The Clipping rect of this Widget.
    /// Most Widgets will forward the Clipping of their parent (which is the default), but some will introduce their own
    /// Clipping rects.
    virtual const Aabrf& get_clipping_rect() const; // TODO: Do we need Widget::Clipping? or similar?

    /// Calculates a transformation from this to another Widget.
    /// @param target    Widget into which to transform.
    /// @throw           GraphError if the two Widgets are not in the same Scene.
    M3f get_xform_to(WidgetHandle target) const;

    /// Sets the space a Widget is "granted" in the Layout of its parent Widget.
    /// @param grant    New Grant.
    void set_grant(Size2f grant);

    // state machine ----------------------------------------------------------

    /// The name of the current State.
    virtual std::string_view get_state_name() const noexcept = 0;

    /// Checks if a transition from one to the other State is possible.
    virtual bool is_valid_transition(const std::string& from, std::string& to) const = 0;

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    virtual void transition_into(const std::string& state) = 0;

protected:
    /// Updates the Design of this Widget through the given Painter.
    virtual void _paint(Painter& painter) const = 0;

    /// Relayout this Widget and all of its direct children.
    virtual void _relayout() = 0;

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    virtual void _get_widgets_at(const V2f& local_pos, std::vector<WidgetHandle>& result) const = 0;

    /// Let subclasses update their Claim whenever they feel like it.
    /// Every change will cause a chain of updates to propagate up and down the Widget hierarchy. If you can, try to
    /// limit the number of times this function is called each frame.
    void _set_claim(WidgetClaim claim);

    /// Sets the layout transformation of a child widget.
    /// Does not cause a re-layout of the Widget hierarchy below the child.
    void _set_child_xform(WidgetHandle& widget, M3f xform);

    /// Reactive function clearing this Widget's design whenever a REFRESH Property changes its value.
    RefreshObserverPtr& _get_refresh_observer() { return m_refresh_observer; }

private:
    /// Changing the Claim or the visiblity of a Widget causes a relayout further up the hierarchy.
    void _relayout_upwards();

    /// Updates the size and transformations of this and all child Widgets.
    void _relayout_downwards();

    /// Recalculates the bounding rect of all descendants of this Widget.
    /// By default, this function simply forms the union of all first level children of a Widget.
    /// It is virtual though, so you can define a function better suited to your specific Widget type (like a quadtree
    /// or whatever).
    virtual Aabrf _calculate_content_aabr() const;

    /// Recalculates the Claim of this Widget.
    /// Virtual for example, for when the Widget's Claim is determined by its children.
    /// Note this method is const, you are not supposed to actually update the Claim, just recalculate it.
    virtual WidgetClaim _calculate_claim() const { return m_claim; }

    /// Updates (if necessary) and returns the Design of this Widget.
    const PlotterDesign& _get_design();

    /// Calculates the transformation of this Widget relative to its Window.
    void _get_window_xform(M3f& result) const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Design of this Widget.
    PlotterDesign m_design;

    /// The Claim of a Widget determines how much space it receives in the parent Layout.
    /// Claim values are in untransformed local space.
    WidgetClaim m_claim;

    /// 2D transformation of this Widget as determined by its parent Layout.
    M3f m_layout_xform = M3f::identity();

    /// The bounding rect of all descendant Widgets.
    Aabrf m_content_aabr = Aabrf::zero();

    /// The grant of a Widget is how much space is 'granted' to it by its parent Layout.
    /// Depending on the parent Layout, the Widget's Claim can be used to influence the grant.
    /// Note that the grant can also be smaller or bigger than the Claim.
    Size2f m_grant = Size2f::zero();

    /// Reactive function clearing this Widget's design whenever a REFRESH Property changes its value.
    RefreshObserverPtr m_refresh_observer = std::make_shared<RefreshObserver>(*this);
};

template<>
inline M3f AnyWidget::get_xform<AnyWidget::Space::LOCAL>() const {
    return M3f::identity();
}

template<>
inline M3f AnyWidget::get_xform<AnyWidget::Space::OFFSET>() const {
    return get<offset_xform>();
}

template<>
inline M3f AnyWidget::get_xform<AnyWidget::Space::LAYOUT>() const {
    return m_layout_xform;
}

template<>
inline M3f AnyWidget::get_xform<AnyWidget::Space::PARENT>() const {
    return get<offset_xform>() * m_layout_xform;
}

template<>
M3f AnyWidget::get_xform<AnyWidget::Space::WINDOW>() const;

// widget handle ==================================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<AnyWidget> : public NodeHandleBaseInterface<AnyWidget> {

    // layout -----------------------------------------------------------------

    using AnyWidget::get_aabr;
    using AnyWidget::get_clipping_rect;
    using AnyWidget::get_xform;
    using AnyWidget::get_xform_to;

    // state machine ----------------------------------------------------------

    using AnyWidget::get_state_name;
    using AnyWidget::is_valid_transition;
    using AnyWidget::transition_into;
};

} // namespace detail

class WidgetHandle : public NodeHandle<AnyWidget> {

    friend Accessor<WidgetHandle, AnyWidget>;
    friend Accessor<WidgetHandle, WidgetVisualizer>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(WidgetHandle);

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using NodeHandle<AnyWidget>::NodeHandle;

    /// Constructor from specialized base.
    WidgetHandle(NodeHandle<AnyWidget>&& handle) : NodeHandle(std::move(handle)) {}

private:
    /// Returns the Widget contained in this Handle.
    AnyWidget* _get_widget() { return _get_node().get(); }

    /// Updates (if necessary) and returns the Design of this Widget.
    const PlotterDesign& _get_design() const { return _get_node()->_get_design(); }
};

// widget handle accessors ========================================================================================== //

template<>
class Accessor<WidgetHandle, AnyWidget> {
    friend AnyWidget;

    /// Returns the Widget contained in this Handle.
    static AnyWidget* get_widget(WidgetHandle& widget) { return widget._get_widget(); }
};

template<>
class Accessor<WidgetHandle, WidgetVisualizer> {
    friend WidgetVisualizer;

    /// Updates (if necessary) and returns the Design of this Widget.
    static const PlotterDesign& get_design(WidgetHandle& widget) { return widget._get_design(); }
};

NOTF_CLOSE_NAMESPACE
