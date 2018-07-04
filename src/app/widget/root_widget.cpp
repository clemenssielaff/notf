#include "app/widget/root_widget.hpp"

#include "app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

RootWidget::RootWidget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent) : Widget(token, scene, parent)
{
    connect_signal(get_property<Size2f>("grant").get_signal(), &RootWidget::_on_grant_changed);
}

RootWidget::~RootWidget() = default;

void RootWidget::_get_widgets_at(const Vector2f& local_pos, std::vector<valid_ptr<Widget*>>& result) const
{
    // TODO: RootWidget::_get_widgets_at
}

void RootWidget::_on_grant_changed(const Size2f& new_grant)
{
    const Vector2f translation = Vector2f(new_grant.width / 2.f, new_grant.height / 2.f);
    m_clipping.set_rect(Aabrf(-translation, new_grant));
    m_clipping.set_xform(Matrix3f::translation(translation));
}

NOTF_CLOSE_NAMESPACE
