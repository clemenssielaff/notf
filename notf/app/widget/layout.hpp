#pragma once

#include <vector>

#include "notf/common/geo/alignment.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/common/geo/padding.hpp"

#include "notf/app/widget/widget_claim.hpp"

NOTF_OPEN_NAMESPACE

// any layout ======================================================================================================= //

/// Every Widget has a Layout that determines how its child widgets are placed. It is a separate class from the Widget,
/// since the Layout of a Widget may change. For example, you might want to have different layouts depending on the size
/// of the Widget, its rotation or whatever else.
/// Also, having Layouts be a separate thing opens the door for layout animations, where the same child widget can be
/// transformed by two Layouts and the eventual transformation be a blend between the two.
class AnyLayout {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Result for a single child widget when the Layout is updated.
    struct Placement {

        /// Transformation of the child widget.
        M3f xform;

        /// How much space is granted to the child widget.
        Size2f grant;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value Constructor.
    /// @param widget   Widget owning this Subscriber.
    AnyLayout(AnyWidget& widget) : m_widget(widget) {}

    /// Destructor.
    virtual ~AnyLayout();

    /// The name of this type of Layout.
    virtual std::string_view get_type_name() const = 0;

    /// Calculates the combined Claim of all of the Widget's children as determined by this Layout.
    /// @param claims   Claims of all child widgets that need to be combined into a single Claim.
    virtual WidgetClaim calculate_claim(const std::vector<const WidgetClaim*> claims) const = 0;

    // TODO: replace the vector of pointers with a vector of valid_ptr?

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    virtual std::vector<Placement> update(const std::vector<const WidgetClaim*> claims, const Size2f& grant) const = 0;

    /// Padding around the Layout's borders.
    const Paddingf& get_padding() const noexcept { return m_padding; }

    /// Defines the padding around the Layout's border.
    /// @param padding  New padding.
    /// @param relayout Whether or not to force a relayout of the Widget after this change. Set this to false if you
    ///                 are changing more than one value and don't want to relayout multiple times.
    void set_padding(Paddingf padding, bool relayout = true) {
        if (padding == m_padding) { return; }
        m_padding = std::move(padding);
        if (relayout) { _relayout(); }
    }

protected:
    /// Lets the Widget know to relayout downwards because something about its layout changed.
    void _relayout();

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Widget whose children are transformed using this Layout.
    AnyWidget& m_widget;

    /// Padding around the Layout's borders.
    Paddingf m_padding = Paddingf::none();
};

// overlayout ======================================================================================================= //

/// The Overlayout stacks all of its children on top of each other.
/// It is the workhorse layout and the basis for many other, more complex structures.
class OverLayout : public AnyLayout {

public:
    /// Constructor.
    /// @param widget   Widget owning this Layout.
    OverLayout(AnyWidget& widget) : AnyLayout(widget) {}

    /// The name of this type of Layout.
    std::string_view get_type_name() const final { return "OverLayout"; }

    /// The combined claim of an OverLayout is the maximum Claim of all of its children.
    /// @param claims   Claims of all child widgets that need to be combined into a single Claim.
    WidgetClaim calculate_claim(const std::vector<const WidgetClaim*> claims) const final;

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    std::vector<Placement> update(const std::vector<const WidgetClaim*> claims, const Size2f& grant) const final;

    /// Alignment of items in the Layout relative to the parent.
    const Alignment& get_alignment() const noexcept { return m_alignment; }

    /// Updates the alignment of the widgets contained in the Layout..
    /// @param alignment    New alignment.
    /// @param relayout     Whether or not to force a relayout of the Widget after this change. Set this to false if
    ///                     you are changing more than one value and don't want to relayout multiple times.
    void set_alignment(Alignment alignment, bool relayout = true) {
        if (alignment == m_alignment) { return; }
        m_alignment = std::move(alignment);
        if (relayout) { _relayout(); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Alignment of items in the Layout relative to the parent.
    Alignment m_alignment = Alignment::center();
};

// flex layout ====================================================================================================== //

/// The FlexLayout class offers a way to arrange items in one or multiple rows or columns.
/// It behaves similar to the CSS Flex Box Layout.
///
/// Claim
/// =====
/// The FlexLayout calculates its implicit Claim differently, depending on whether it is wrapping or not.
/// A non-wrapping FlexLayout will simply add all of its child Claim's either vertically or horizontally.
/// A wrapping FlexLayout on the other hand, will *always* return the default Claim (0 min/preferred, inf max).
/// This is because a wrapping FlexLayout will most likely be used to arrange elements within an explicitly defined area
/// that is not determined its child items themselves.
/// By using the default Claim, it allows its parent to determine its size explicitly and arrange its children within
/// the alloted size.
class FlexLayout : public AnyLayout {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Direction in which items in a Layout can be stacked.
    enum class Direction {
        LEFT_TO_RIGHT,
        TOP_TO_BOTTOM,
        RIGHT_TO_LEFT,
        BOTTOM_TO_TOP,
    };

    /// Alignment of items in a FlexLayout along the main and cross axis.
    enum class Alignment : uchar {
        START,         // items stacked from the start, no additional spacing
        END,           // items stacked from the end, no additional spacing
        CENTER,        // items centered, no additional spacing
        SPACE_BETWEEN, // equal spacing between items, no spacing between items and border
        SPACE_EQUAL,   // equal spacing between the items and the border
        SPACE_AROUND,  // single spacing between items and border, double spacing between items
    };

    /// How the FlexLayout wraps.
    enum class Wrap : uchar {
        NO_WRAP, // no wrap
        WRAP,    // wraps towards the lower-right corner
        REVERSE, // wraps towards the upper-left corner
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param widget   Widget owning this Layout.
    FlexLayout(AnyWidget& widget) : AnyLayout(widget) {}

    /// The name of this type of Layout.
    std::string_view get_type_name() const final { return "FlexLayout"; }

    /// The combined claim of an OverLayout is the maximum Claim of all of its children.
    /// @param claims   Claims of all child widgets that need to be combined into a single Claim.
    WidgetClaim calculate_claim(const std::vector<const WidgetClaim*> claims) const final;

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    std::vector<Placement> update(const std::vector<const WidgetClaim*> claims, const Size2f& grant) const final;

    /// Direction in which items are stacked.
    Direction get_direction() const { return m_direction; }

    /// Defines the direction in which the FlexLayout is stacked.
    /// @param direction    New direction.
    /// @param relayout     Whether or not to force a relayout of the Widget after this change. Set this to false if
    ///                     you are changing more than one value and don't want to relayout multiple times.
    void set_direction(const Direction direction, bool relayout = true) {
        if (direction == m_direction) { return; }
        m_direction = direction;
        if (relayout) { _relayout(); }
    }

    /// Alignment of items in the main direction.
    Alignment get_alignment() const { return m_main_alignment; }

    /// Defines the alignment of stack items in the main direction.
    /// @param alignment    New alignment.
    /// @param relayout     Whether or not to force a relayout of the Widget after this change. Set this to false if
    ///                     you are changing more than one value and don't want to relayout multiple times.
    void set_alignment(const Alignment alignment, bool relayout = true) {
        if (alignment == m_main_alignment) { return; }
        m_main_alignment = alignment;
        if (relayout) { _relayout(); }
    }

    /// Alignment of items in the cross direction.
    /// There are only 3 relevant states: START / END and CENTER. Everything else is treated like CENTER.
    Alignment get_cross_alignment() const { return m_cross_alignment; }

    /// Defines the alignment of stack items in the cross direction.
    /// There are only 3 relevant states: START / END and CENTER. Everything else is treated like CENTER.
    /// @param alignment    New cross alignment.
    /// @param relayout     Whether or not to force a relayout of the Widget after this change. Set this to false if
    ///                     you are changing more than one value and don't want to relayout multiple times.
    void set_cross_alignment(Alignment alignment, bool relayout = true) {
        if (alignment == Alignment::SPACE_BETWEEN   //
            || alignment == Alignment::SPACE_AROUND //
            || alignment == Alignment::SPACE_EQUAL) {
            alignment = Alignment::CENTER;
        }
        if (alignment == m_cross_alignment) { return; }
        m_cross_alignment = alignment;
        if (relayout) { _relayout(); }
    }

    /// Cross alignment of the individual stacks if the Layout wraps.
    Alignment get_stack_alignment() const { return m_stack_alignment; }

    /// Defines the cross alignment of the individual stacks if the Layout wraps.
    /// @param alignment    New stack alignment.
    /// @param relayout     Whether or not to force a relayout of the Widget after this change. Set this to false if
    ///                     you are changing more than one value and don't want to relayout multiple times.
    void set_stack_alignment(const Alignment alignment, bool relayout = true) {
        if (alignment == m_stack_alignment) { return; }
        m_stack_alignment = alignment;
        if (relayout) { _relayout(); }
    }

    /// How (and if) overflowing lines are wrapped.
    Wrap get_wrap() const { return m_wrap; }

    /// Defines how (and if) overflowing lines are wrapped.
    /// @param wrap     New wrap.
    /// @param relayout Whether or not to force a relayout of the Widget after this change. Set this to false if you
    ///                 are changing more than one value and don't want to relayout multiple times.
    void set_wrap(const Wrap wrap, bool relayout = true) {
        if (wrap == m_wrap) { return; }
        m_wrap = wrap;
        if (relayout) { _relayout(); }
    }

    /// True if overflowing lines are wrapped.
    bool is_wrapping() const { return m_wrap != Wrap::NO_WRAP; }

    /// Tests whether a FlexLayout stacks horizontally.
    bool is_horizontal() const {
        return (m_direction == FlexLayout::Direction::LEFT_TO_RIGHT
                || m_direction == FlexLayout::Direction::RIGHT_TO_LEFT);
    }

    /// Tests whether a FlexLayout stacks vertically.
    bool is_vertical() const {
        return (m_direction == FlexLayout::Direction::TOP_TO_BOTTOM
                || m_direction == FlexLayout::Direction::BOTTOM_TO_TOP);
    }

    /// Additional spacing between items in the main direction.
    float get_spacing() const { return m_spacing; }

    /// Defines additional spacing between items.
    /// @param spacing  New spacing.
    /// @param relayout Whether or not to force a relayout of the Widget after this change. Set this to false if you
    ///                 are changing more than one value and don't want to relayout multiple times.
    void set_spacing(float spacing, bool relayout = true) {
        spacing = spacing < 0 ? 0 : spacing;
        if (is_approx(spacing, m_spacing)) { return; }
        m_spacing = spacing;
        if (relayout) { _relayout(); }
    }

    /// Spacing between stacks of items if this Layout is wrapped.
    float get_cross_spacing() const { return m_cross_spacing; }

    /// Defines the spacing between stacks of items if this Layout is wrapped.
    /// @param spacing  New cross spacing.
    /// @param relayout Whether or not to force a relayout of the Widget after this change. Set this to false if you
    ///                 are changing more than one value and don't want to relayout multiple times.
    void set_cross_spacing(float spacing, bool relayout = true) {
        spacing = spacing < 0 ? 0 : spacing;
        if (is_approx(spacing, m_cross_spacing)) { return; }
        m_cross_spacing = spacing;
        if (relayout) { _relayout(); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Spacing between items in the Layout in the main direction.
    float m_spacing = 0;

    /// Spacing between stacks, if this Layout wraps.
    float m_cross_spacing = 0;

    /// Direction in which the FlexLayout is stacked.
    Direction m_direction = Direction::LEFT_TO_RIGHT;

    /// How items in the Layout are wrapped.
    Wrap m_wrap = Wrap::NO_WRAP;

    /// Alignment of items in the main direction.
    Alignment m_main_alignment = Alignment::START;

    /// Alignment of items in the cross direction.
    Alignment m_cross_alignment = Alignment::START;

    /// Cross alignment of the individual stacks if the Layout wraps.
    Alignment m_stack_alignment = Alignment::START;
};

NOTF_CLOSE_NAMESPACE
