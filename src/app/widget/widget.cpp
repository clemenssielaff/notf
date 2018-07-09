#include "app/widget/widget_scene.hpp"

#include "app/widget/painter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Widget::Widget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent, const std::string& name)
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
    m_grant = _create_property<Size2f>("grant", Size2f::zero(), {}, /* has_body = */ false);
    m_visibility = _create_property<bool>("visibility", true);
    m_opacity = _create_property<float>("opacity", 1, [](float& v) {
        v = clamp(v, 0, 1);
        return true;
    });

    // connect callbacks
    connect_signal(m_visibility.get_signal(), &Widget::_relayout_upwards);
    connect_signal(m_claim.get_signal(), &Widget::_relayout_upwards);
    connect_signal(m_grant.get_signal(), &Widget::_relayout_downwards);
}

Widget::~Widget() = default;

Matrix3f Widget::get_xform_to(valid_ptr<const Widget*> target) const
{
    NodeHandle<Widget> common_ancestor = get_common_ancestor(target);
    if (!common_ancestor) {
        NOTF_THROW(runtime_error, "Cannot find common ancestor for Widgets \"{}\" and \"{}\"", get_name(),
                   target->get_name());
    }

    Matrix3f source_branch = Matrix3f::identity();
    for (const Widget* it = this; it != common_ancestor; it = assert_cast<Widget*>(it->_get_parent())) {
        source_branch *= it->get_xform<Space::PARENT>();
    }

    Matrix3f target_branch = Matrix3f::identity();
    for (const Widget* it = target; it != common_ancestor; it = assert_cast<Widget*>(it->_get_parent())) {
        target_branch *= it->get_xform<Space::PARENT>();
    }
    target_branch.inverse();

    source_branch *= target_branch;
    return source_branch;
}

const Clipping& Widget::get_clipping_rect() const { return assert_cast<Widget*>(_get_parent())->get_clipping_rect(); }

void Widget::_relayout_upwards()
{
    Widget* parent = dynamic_cast<Widget*>(raw_pointer(_get_parent()));
    if (!parent) {
        return; // only the root Widget has no parent Widget
    }

    Claim new_parent_claim = parent->_calculate_claim();
    if (new_parent_claim == parent->m_claim.get()) {
        // this Widget's Claim changed, but the parent's Claim stayed the same
        // therefore the parent needs to re-layout its children
        parent->_relayout_downwards();
    }
    else {
        // this Widget's Claim changed, and the parent's Claim will have to change as a result
        // therefore, the parent needs to update its Claim and continue the propagation of the change up the hierarchy
        parent->m_claim.set(std::move(new_parent_claim));
    }
}

const WidgetDesign& Widget::get_design()
{
    Painter painter(m_design);
    _paint(painter); // TODO: minimize widget redesign
    return m_design;
}

void Widget::_get_window_xform(Matrix3f& result) const
{
    if (valid_ptr<Widget*> parent = dynamic_cast<Widget*>(raw_pointer(_get_parent()))) {
        parent->_get_window_xform(result);
        result.premult(get_xform<Space::PARENT>());
    }
}

template<>
const Matrix3f Widget::get_xform<Widget::Space::WINDOW>() const
{
    Matrix3f result = Matrix3f::identity();
    _get_window_xform(result);
    return result;
}

NOTF_CLOSE_NAMESPACE
