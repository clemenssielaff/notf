#include "app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetScene::~WidgetScene() = default;

void WidgetScene::_resize_view(Size2i) {}

bool WidgetScene::_handle_event(Event& /*event*/) { return true; }

NOTF_CLOSE_NAMESPACE
