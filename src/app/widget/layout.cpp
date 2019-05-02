#include "notf/app/widget/layout.hpp"

NOTF_OPEN_NAMESPACE

// any layout ======================================================================================================= //

AnyLayout::~AnyLayout() = default;

// overlayout ======================================================================================================= //

WidgetClaim OverLayout::calculate_claim(const std::vector<const WidgetClaim*> child_claims) const {
    WidgetClaim result;
    for (const WidgetClaim* claim : child_claims) {
        NOTF_ASSERT(claim);
        result.maxed(*claim);
    }
    return result;
}

std::vector<AnyLayout::Placement> OverLayout::update(const std::vector<const WidgetClaim*> child_claims,
                                                     const Size2f& grant) const {
    const Size2f available_size = {grant.width() - m_padding.get_width(), grant.height() - m_padding.get_height()};

    std::vector<Placement> placements(child_claims.size());
    for (size_t i = 0; i < child_claims.size(); ++i) {
        NOTF_ASSERT(child_claims[i]);
        const WidgetClaim& claim = *child_claims[i];
        Placement& placement = placements[i];

        // every widget can claim as much of the available size as it wants
        placement.grant = claim.apply(available_size);

        // the placement of the widget depends on how much it size it claimed
        V2f position;
        position.x() = (max(0, available_size.width() - placement.grant.width()) * m_alignment.horizontal.get_value())
                       + m_padding.left;
        position.y() = (max(0, available_size.height() - placement.grant.height()) * m_alignment.vertical.get_value())
                       + m_padding.bottom;
        placement.xform = M3f::translation(std::move(position));
    }
    return placements;
}

NOTF_CLOSE_NAMESPACE
