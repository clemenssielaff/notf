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
    /// @param child_claims Claims of all child widgets that need to be combined into a single Claim.
    virtual WidgetClaim calculate_claim(const std::vector<const WidgetClaim*> child_claims) const = 0;

    // TODO: replace the vector of pointers with a vector of valid_ptr?

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    virtual std::vector<Placement> update(const std::vector<const WidgetClaim*> child_claims, const Size2f& grant) const = 0;

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Widget whose children are transformed using this Layout.
    const AnyWidget& m_widget;
};

// overlayout ======================================================================================================= //

/// The Overlayout stacks all of its children on top of each other.
/// It is the workhorse layout and the basis for many other, more complex structures.
class OverLayout : public AnyLayout {

public:
    /// Value Constructor.
    /// @param widget   Widget owning this Subscriber.
    OverLayout(AnyWidget& widget) : AnyLayout(widget) {}

    /// The name of this type of Layout.
    std::string_view get_type_name() const final { return "OverLayout"; }

    /// The combined claim of an OverLayout is the maximum Claim of all of its children.
    /// @param child_claims Claims of all child widgets that need to be combined into a single Claim.
    WidgetClaim calculate_claim(const std::vector<const WidgetClaim*> child_claims) const final;

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    std::vector<Placement> update(const std::vector<const WidgetClaim*> child_claims, const Size2f& grant) const final;

    /// Padding around the Layout's borders.
    const Paddingf& get_padding() const noexcept { return m_padding; }

    /// Defines the padding around the Layout's border.
    /// @param padding  New padding.
    void set_padding(Paddingf padding) { m_padding = std::move(padding); }

    /// Alignment of items in the Layout relative to the parent.
    const Alignment& get_alignment() const noexcept { return m_alignment; }

    /// Updates the padding around the Layout's border.
    /// @param alignment    New alignment.
    void set_alignment(Alignment alignment) { m_alignment = std::move(alignment); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Padding around the Layout's borders.
    Paddingf m_padding = Paddingf::none();

    /// Alignment of items in the Layout relative to the parent.
    Alignment m_alignment = Alignment::center();
};

NOTF_CLOSE_NAMESPACE
