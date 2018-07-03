#include "app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Widget::~Widget() = default;

void Widget::_relayout(){};

Clipping Widget::get_clipping_rect() const
{
    return assert_cast<Widget*>(_get_parent())->get_clipping_rect();

    //    risky_ptr<WindowPtr> window = get_graph().get_window();
    //    if (!window) {
    //        m_clipping = {};
    //        return m_clipping;
    //    }

    //    // update clipping rect
    //    const Size2i window_size = window->get_buffer_size();
    //    const Vector2f translation = Vector2f(window_size.width / 2., window_size.height / 2.);
    //    m_clipping.rect = Aabrf(-translation, window_size);
    //    m_clipping.xform = Matrix3f::translation(translation);

    //    return m_clipping;
}

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
