#pragma once

#include <vector>

#include "notf/common/geo/matrix3.hpp"

#include "notf/app/widget/widget_claim.hpp"

NOTF_OPEN_NAMESPACE

// any layout ======================================================================================================= //

class AnyLayout {

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
    AnyLayout(AnyWidget& widget) : m_widget(widget) {}

    /// Destructor.
    virtual ~AnyLayout() = default;

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

// overlayout ======================================================================================================= //

class NoLayout : public AnyLayout { // TODO: NoLayout is not a solution

public:
    /// Value Constructor.
    /// @param widget   Widget owning this Subscriber.
    NoLayout(AnyWidget& widget) : AnyLayout(widget) {}

    /// The name of this type of Layout.
    std::string_view get_type_name() const final { return "NoLayout"; }

    /// Calculates the combined Claim of all of the Widget's children as determined by this Layout.
    /// @param child_claims Claims of all child widgets that need to be combined into a single Claim.
    WidgetClaim calculate_claim(const ClaimList child_claims) const final { return WidgetClaim(); }

    /// @param grant    Size available for the Layout to place the child widgets.
    /// @returns        The bounding rect of all descendants of this Widget.
    std::vector<Placement> update(const ClaimList child_claims, const Size2f& grant) const final {
        std::vector<Placement> result(child_claims.size());
        for (size_t i = 0; i < child_claims.size(); ++i) {
            result[i].grant = Size2f{child_claims[i]->get_horizontal().get_preferred(),
                                     child_claims[i]->get_vertical().get_preferred()};
            // TODO: Size2f Claim::get_preferred
        }
        return result;
    }
};


NOTF_CLOSE_NAMESPACE
