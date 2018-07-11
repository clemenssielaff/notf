#include "app/widget/widget_scene.hpp"

#include "app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetScene::WidgetScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
    : Scene(token, graph, std::move(name)), m_root_widget(_get_root().set_child<RootWidget>())
{}

WidgetScene::~WidgetScene() = default;

void WidgetScene::_resize_view(Size2i) {}

bool WidgetScene::_handle_event(Event& /*event*/) { return true; }

NOTF_CLOSE_NAMESPACE
