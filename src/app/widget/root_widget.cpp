#include "app/widget/root_widget.hpp"

#include "app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

RootWidget::RootWidget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent) : Widget(token, scene, parent) {}

RootWidget::~RootWidget() = default;

void RootWidget::_get_widgets_at(const Vector2f& local_pos, std::vector<valid_ptr<Widget*>>& result) const {}

NOTF_CLOSE_NAMESPACE
