#include "notf/app/widget/any_widget.hpp"

#include "notf/graphic/plotter/painter.hpp"

#include "notf/reactive/trigger.hpp"

#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// any widget ======================================================================================================= //

AnyWidget::AnyWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {

    // set up properties
    _set_property_callback<opacity>([](float& value) {
        value = clamp(value, 0, 1);
        return true;
    });
    connect_property<visibility>()->subscribe(Trigger([this](const bool&) { this->_relayout_upwards(); }));

    // TODO: always have a layout instance for a widget, maybe a StackLayout?
}

M3f AnyWidget::get_xform_to(WidgetHandle target) const {
    AnyWidget* other = static_cast<AnyWidget*>(AnyNodeHandle::AccessFor<AnyWidget>::get_node_ptr(target).get());
    const AnyWidget* common_ancestor
        = dynamic_cast<const AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_common_ancestor(*this, other));
    if (!common_ancestor) {
        NOTF_THROW(GraphError, "Nodes \"{}\" and \"{}\" are not in the same Scene", get_name(), target.get_name());
    }

    M3f source_branch = M3f::identity();
    for (const AnyWidget* itr = this; itr != common_ancestor;
         itr = dynamic_cast<const AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*itr))) {
        if (!itr) {
            NOTF_THROW(GraphError, "Nodes \"{}\" and \"{}\" are not in the same Scene", get_name(), target.get_name());
        }
        source_branch *= itr->get_xform();
    }

    M3f target_branch = M3f::identity();
    for (const AnyWidget* itr = this; itr != common_ancestor;
         itr = dynamic_cast<const AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*itr))) {
        if (!itr) {
            NOTF_THROW(GraphError, "Nodes \"{}\" and \"{}\" are not in the same Scene", get_name(), target.get_name());
        }
        target_branch *= itr->get_xform();
    }
    target_branch = target_branch.get_inverse();

    source_branch *= target_branch;
    return source_branch;
}

const Aabrf& AnyWidget::get_clipping_rect() const {
    if (AnyWidget* parent = dynamic_cast<AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this))) {
        return parent->get_clipping_rect();
    } else {
        WidgetScene* scene = dynamic_cast<WidgetScene*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this));
        NOTF_ASSERT(scene);
        return scene->get_clipping_rect();
    }
}

void AnyWidget::set_claim(WidgetClaim claim) {
    m_is_claim_explicit = true;
    _set_claim(std::move(claim));
}

void AnyWidget::unset_claim() {
    if (m_is_claim_explicit) {
        m_is_claim_explicit = false;
        const auto [_, claim_list] = _get_claim_list();
        _set_claim(m_layout->calculate_claim(claim_list));
    }
}

void AnyWidget::_set_claim(WidgetClaim claim) {
    if (claim != m_claim) {
        m_claim = std::move(claim);
        _relayout_upwards();
    }
}

void AnyWidget::_set_grant(Size2f grant) {
    if (grant != m_grant) {
        m_grant = std::move(grant);
        _relayout_downwards();
    }
}

std::pair<std::vector<AnyWidget*>, AnyWidget::Layout::ClaimList> AnyWidget::_get_claim_list() {
    std::vector<AnyWidget*> child_widgets;
    child_widgets.reserve(get_child_count());
    for (size_t i = 0; i < get_child_count(); ++i) {
        // TODO: can we make sure that widgets can only parent other widgets? Or can they in fact parent other things?
        if (AnyWidget* child
            = dynamic_cast<AnyWidget*>(AnyNodeHandle::AccessFor<AnyWidget>::get_node_ptr(get_child(i)).get())) {
            child_widgets.emplace_back(child);
        }
    }

    std::vector<const WidgetClaim*> child_claims;
    child_claims.reserve(child_widgets.size());
    for (AnyWidget* child : child_widgets) {
        child_claims.push_back(&child->m_claim);
    }

    return std::make_pair(std::move(child_widgets), std::move(child_claims));
}

void AnyWidget::_relayout_upwards() {
    AnyWidget* parent = dynamic_cast<AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this));
    if (!parent) {
        // if the parent is not a widget, this is the root widget
        _relayout_downwards();
    } else if (parent->m_is_claim_explicit) {
        // if the parent's claim is explicit, it will not update its own claim and only needs to re-layout its children
        parent->_relayout_downwards();
    } else {
        const auto [_, claim_list] = _get_claim_list();
        WidgetClaim new_parent_claim = parent->get_layout().calculate_claim(claim_list);
        if (new_parent_claim == parent->m_claim) {
            // if the parent did not update its Claim, it needs to re-layout its children
            parent->_relayout_downwards();
        } else {
            // if the parent's Claim changed as well, we need to propagate the update up the hierarchy
            parent->_set_claim(std::move(new_parent_claim));
        }
    }
}

void AnyWidget::_relayout_downwards() {
    auto [child_widgets, claim_list] = _get_claim_list();
    const std::vector<Layout::Placement> placements = m_layout->update(claim_list, m_grant);
    NOTF_ASSERT(placements.size() == child_widgets.size());

    m_children_aabr = Aabrf::zero();
    for (size_t i = 0; i < child_widgets.size(); ++i) {
        AnyWidget* child = child_widgets[i];
        NOTF_ASSERT(child);

        // update the child's layout placement
        child->m_layout_xform = std::move(placements[i].xform);
        child->_set_grant(std::move(placements[i].grant));

        // update the children aabr
        m_children_aabr.unite(transform_by(child->get_aabr(), child->get_xform()));
    }
}

const PlotterDesign& AnyWidget::_get_design() {
    if (m_design.is_dirty()) {
        Painter painter(m_design);
        _paint(painter);
    }
    return m_design;
}

void AnyWidget::_get_window_xform(M3f& result) const {
    if (AnyWidget* parent = dynamic_cast<AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this))) {
        parent->_get_window_xform(result);
    }
    result *= get_xform();
}

NOTF_CLOSE_NAMESPACE
