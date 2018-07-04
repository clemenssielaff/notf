#include "app/widget/widget_scene.hpp"

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
    m_claim = _create_property<Claim>("claim", Claim(), [&](Claim& claim) { return _set_claim(claim); },
                                      /* has_body = */ false);
    m_layout_transform = _create_property<Matrix3f>("layout_transform", Matrix3f::identity());
    m_offset_transform = _create_property<Matrix3f>("offset_transform", Matrix3f::identity());
    m_content_aabr = _create_property<Aabrf>("content_aabr", Aabrf::zero(), {}, /* has_body = */ false);
    m_grant = _create_property<Size2f>("grant", Size2f::zero(), [&](Size2f& grant) { return _set_grant(grant); },
                                       /* has_body = */ false);
    m_opacity = _create_property<float>("opacity", 1, [](float& v) {
        v = clamp(v, 0, 1);
        return true;
    });
    m_visibility = _create_property<bool>("visibility", true);
}

Widget::~Widget() = default;



bool Widget::_set_claim(Claim& claim)
{
    // TODO: Widget::_set_claim
    return true;
}

bool Widget::_set_grant(Size2f& grant)
{
    // TODO: Widget::_set_grant
    return true;
}

void Widget::_relayout(){};

const Clipping& Widget::get_clipping_rect() const { return assert_cast<Widget*>(_get_parent())->get_clipping_rect(); }

Matrix3f Widget::get_xform_to(valid_ptr<const Widget*> target) const
{
    NodeHandle<Widget> common_ancestor = get_common_ancestor(target);
    if (!common_ancestor) {
        notf_throw(runtime_error, "Cannot find common ancestor for Widgets \"{}\" and \"{}\"", get_name(),
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

void Widget::_get_window_xform(Matrix3f& result) const
{
    if (valid_ptr<Widget*> parent = dynamic_cast<Widget*>(raw_pointer(_get_parent()))) {
        parent->_get_window_xform(result);
        result.premult(get_xform<Space::PARENT>());
    }
}

NOTF_CLOSE_NAMESPACE
