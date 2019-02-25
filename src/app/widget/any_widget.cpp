#include "notf/app/widget/any_widget.hpp"

#include "notf/graphic/plotter/painter.hpp"

#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// any widget ======================================================================================================= //

AnyWidget::AnyWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {

    // set up properties
    _set_property_callback<opacity>([](float& value) {
        value = clamp(value, 0, 1);
        return true;
    });
    //    connect_signal(m_visibility.get_signal(), &Widget::_relayout_upwards);
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
        source_branch *= itr->get_xform<Space::PARENT>();
    }

    M3f target_branch = M3f::identity();
    for (const AnyWidget* itr = this; itr != common_ancestor;
         itr = dynamic_cast<const AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*itr))) {
        if (!itr) {
            NOTF_THROW(GraphError, "Nodes \"{}\" and \"{}\" are not in the same Scene", get_name(), target.get_name());
        }
        target_branch *= itr->get_xform<Space::PARENT>();
    }
    target_branch = target_branch.get_inverse();

    source_branch *= target_branch;
    return source_branch;
}

void AnyWidget::set_grant(Size2f grant) {
    if (grant != m_grant) {
        m_grant = std::move(grant);
        _relayout_downwards();
    }
}

void AnyWidget::_set_claim(WidgetClaim claim) {
    if (claim != m_claim) {
        m_claim = std::move(claim);
        _relayout_upwards();
    }
}

void AnyWidget::_set_child_xform(WidgetHandle& widget, M3f xform) {
    if (AnyWidget* widget_ptr = WidgetHandle::AccessFor<AnyWidget>::get_widget(widget)) {
        widget_ptr->m_layout_xform = std::move(xform);
    }
}

void AnyWidget::_relayout_upwards() {
    AnyWidget* parent = dynamic_cast<AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this));
    if (!parent) {
        return; // only the root Widget has no parent Widget
    }

    WidgetClaim new_parent_claim = parent->_calculate_claim();
    if (new_parent_claim == parent->m_claim) {
        // this Widget's Claim changed, but the parent's Claim stayed the same
        // therefore the parent needs to re-layout its children
        parent->_relayout_downwards();
    } else {
        // this Widget's Claim changed, and the parent's Claim will have to change as a result
        // therefore, the parent needs to update its Claim and continue the propagation of the change up the hierarchy
        parent->_set_claim(std::move(new_parent_claim));
    }
}

void AnyWidget::_relayout_downwards() {
    _relayout();
    m_content_aabr = _calculate_content_aabr();
}

Aabrf AnyWidget::_calculate_content_aabr() const {
    Aabrf result = Aabrf::zero();
    for (size_t i = 0; i < get_child_count(); ++i) {
        if (AnyWidget* child
            = dynamic_cast<AnyWidget*>(AnyNodeHandle::AccessFor<AnyWidget>::get_node_ptr(get_child(i)).get())) {
            result.unite(child->get_aabr<Space::PARENT>());
        }
    }
    return result;
}

const PlotterDesign& AnyWidget::_get_design() {
    Painter painter(m_design);
    _paint(painter); // TODO: minimize widget redesign
    return m_design;
}

void AnyWidget::_get_window_xform(M3f& result) const {
    if (AnyWidget* parent = dynamic_cast<AnyWidget*>(AnyNode::AccessFor<AnyWidget>::get_parent(*this))) {
        parent->_get_window_xform(result);
    }
    result *= get_xform<Space::PARENT>();
}

template<>
M3f AnyWidget::get_xform<AnyWidget::Space::WINDOW>() const {
    M3f result = M3f::identity();
    _get_window_xform(result);
    return result;
}

NOTF_CLOSE_NAMESPACE
