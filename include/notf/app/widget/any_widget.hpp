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

public:
    /// Error thrown when a requested State transition is impossible.
    NOTF_EXCEPTION_TYPE(BadTransitionError);

    /// Property names.
    static constexpr const ConstString& offset_xform = detail::widget_policy::OffsetXform::name;
    static constexpr const ConstString& opacity = detail::widget_policy::Opacity::name;
    static constexpr const ConstString& visibility = detail::widget_policy::Visibility::name;

    // layout -----------------------------------------------------------------
public:
    class Layout {

        // types ----------------------------------------------------------- //
    public:
        /// List of (pointers to) Claims of all child widgets that need to be layed out in draw order.
        using ClaimList = std::vector<const WidgetClaim*>;

        /// Result for a single child widget when the Layout is updated.
        struct Placement {
            M3f xform;
            Size2f grant;
        };

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param widget   Widget owning this Subscriber.
        Layout(AnyWidget& widget) : m_widget(widget) {}

        /// Destructor.
        virtual ~Layout() = default;

        /// The name of this type of Layout.
        virtual std::string_view get_type_name() const = 0;

        /// Calculates the combined Claim of all of the Widget's children as determined by this Layout.
        /// @param child_claims Claims of all child widgets that need to be combined into a single Claim.
        virtual WidgetClaim calculate_claim(const ClaimList child_claims) const = 0;

        /// @param grant    Size available for the Layout to place the child widgets.
        /// @returns        The bounding rect of all descendants of this Widget.
        virtual std::vector<Placement> update(const ClaimList child_claims, const Size2f& grant) const = 0;

        // fields ---------------------------------------------------------- //
    protected:
        /// Widget whose children are transformed using this Layout.
        const AnyWidget& m_widget;
    };
    using LayoutPtr = std::unique_ptr<Layout>;

    // refresh observer -------------------------------------------------------
private:
    /// Internal reactive function that is subscribed to all REFRESH Properties and clears the WidgetDesign, should one
    /// of them change.
    class RefreshObserver : public Subscriber<All> {

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param widget   Widget owning this Subscriber.
        RefreshObserver(AnyWidget& widget) : m_widget(widget) {}

        /// Called whenever a REFRESH Property changed its value.
        void on_next(const AnyPublisher*, ...) final { m_widget.m_design.set_dirty(); }

        // fields ---------------------------------------------------------- //
    private:
        /// Widget owning this Subscriber.
        AnyWidget& m_widget;
    };
    using RefreshObserverPtr = std::shared_ptr<RefreshObserver>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    AnyWidget(valid_ptr<AnyNode*> parent);

    // layout -----------------------------------------------------------------
public:
    /// Widget's transformation in parent space.
    M3f get_xform() const { return get<offset_xform>() * m_layout_xform; }

    /// Widget's transformation in window space.
    M3f get_window_xform() const {
        M3f result = M3f::identity();
        _get_window_xform(result);
        return result;
    }

    /// Calculates a transformation from this to another Widget.
    /// @param target    Widget into which to transform.
    /// @throw           GraphError if the two Widgets are not in the same Scene.
    M3f get_xform_to(WidgetHandle target) const;

    /// The axis-aligned bounding rect around this and all children of this Widget in local space.
    /// Local space means that the content is transformed by the Widget's offset- and layout transformation before it
    /// is displayed in parent space (which itself may be transformed).
    const Aabrf& get_aabr() const { return m_children_aabr; }

    /// A mutable, typed refernce to the layout of this Widget.
    /// Returns the base type by default which should always be valid. If another type is requested, this method might
    /// throw.
    template<class T = Layout>
    T& get_layout() {
        NOTF_ASSERT(m_layout);
        if constexpr (std::is_same_v<T, Layout>) {
            return *m_layout.get();
        } else {
            if (T* layout_ptr = dynamic_cast<T*>(m_layout.get())) { return *layout_ptr; }
            NOTF_THROW(TypeError, "Layout of Widget \"{}\" is not of type \"{}\", but of type \"{}\"", //
                       get_name(), type_name<T>(), m_layout->get_type_name());
        }
    }

    /// The Clipping rect of this Widget.
    /// Most Widgets will forward the Clipping of their parent (which is the default), but some will introduce their own
    /// Clipping rects.
    virtual const Aabrf& get_clipping_rect() const; // TODO: Do we need Widget::Clipping? or similar?

    /// Is true, if the Claim of this Widget is determined by the Widget itself.
    /// Is false, if the Claim of this Widget is determined by the combined Claims of all of its children.
    bool has_explicit_claim() const { return m_is_claim_explicit; }

    /// Set an explicit Claim for this Widget.
    /// @param claim    New, explicit Claim.
    void set_claim(WidgetClaim claim);

    /// Removes an explicit Claim from this Widget.
    /// Afterwards, the Claim of the Widget will be determined by the combined Claims of all of its children.
    void unset_claim();

    // state machine ----------------------------------------------------------
public:
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

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    virtual void _get_widgets_at(const V2f& local_pos, std::vector<WidgetHandle>& result) const = 0;

    /// Reactive function clearing this Widget's design whenever a REFRESH Property changes its value.
    RefreshObserverPtr& _get_refresh_observer() { return m_refresh_observer; }

    /// Let subclasses update their Claim whenever they feel like it.
    /// Every change will cause a chain of updates to propagate up and down the Widget hierarchy. If you can, try to
    /// limit the number of times this function is called each frame.
    void _set_claim(WidgetClaim claim);

    /// Sets the space a Widget is "granted" in the Layout of its parent Widget.
    /// @param grant    New Grant.
    void _set_grant(Size2f grant);

    /// Produces a list of Claims of each child widget in draw order.
    std::pair<std::vector<AnyWidget*>, AnyWidget::Layout::ClaimList> _get_claim_list();

private:
    /// Changing the Claim or the visiblity of a Widget causes a relayout further up the hierarchy.
    void _relayout_upwards();

    /// Updates the size and transformations of this and all child Widgets.
    void _relayout_downwards();

    /// Updates (if necessary) and returns the Design of this Widget.
    const PlotterDesign& _get_design();

    /// Calculates the transformation of this Widget relative to its Window.
    void _get_window_xform(M3f& result) const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Is true, if the Claim of this Widget is determined by the Widget itself.
    /// Is false, if the Claim of this Widget is determined by the combined Claims of all of its children.
    bool m_is_claim_explicit;

    /// Current layout of this Widget.
    LayoutPtr m_layout;

    /// The Claim of a Widget determines how much space it receives in the parent Layout.
    /// Claim values are in untransformed local space.
    WidgetClaim m_claim;

    /// The grant of a Widget is how much space is 'granted' to it by its parent Layout.
    /// Depending on the parent Layout, the Widget's claim can be used to influence the grant.
    /// Note that the grant can also be smaller or bigger than the claim.
    Size2f m_grant = Size2f::zero();

    /// 2D transformation of this Widget as determined by its parent Layout.
    M3f m_layout_xform = M3f::identity();

    /// The union bounding rect of this and all descendant Widgets.
    /// We keep this value around because it is only updated whenever the widget's layout changes but is (probably?)
    /// read more often than that.
    Aabrf m_children_aabr = Aabrf::zero();

    /// Reactive function clearing this Widget's design whenever a REFRESH Property changes its value.
    RefreshObserverPtr m_refresh_observer = std::make_shared<RefreshObserver>(*this);

    /// Design of this Widget.
    PlotterDesign m_design;
};

// widget handle ==================================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<AnyWidget> : public NodeHandleBaseInterface<AnyWidget> {

    // layout -----------------------------------------------------------------

    using AnyWidget::get_aabr;
    using AnyWidget::get_clipping_rect;
    using AnyWidget::get_window_xform;
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
