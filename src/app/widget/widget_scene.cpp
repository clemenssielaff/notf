#include "notf/app/widget/widget_scene.hpp"

#include "notf/app/graph/window.hpp"

namespace {
NOTF_USING_NAMESPACE;

GraphicsContext& get_graphics_context(AnyNode* node) {
    Window* window = dynamic_cast<Window*>(node);
    NOTF_ASSERT(window);
    return window->get_graphics_context();
}

} // namespace

NOTF_OPEN_NAMESPACE

// widget scene ===================================================================================================== //

WidgetScene::WidgetScene(valid_ptr<AnyNode*> parent)
    : Scene(parent, std::make_shared<WidgetVisualizer>(get_graphics_context(parent))) {}

NOTF_CLOSE_NAMESPACE
