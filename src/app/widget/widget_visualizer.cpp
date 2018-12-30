#include "notf/app/widget/widget_visualizer.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/graph/window.hpp"
#include "notf/app/widget/any_widget.hpp"
#include "notf/app/widget/painterpreter.hpp"
#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetVisualizer::WidgetVisualizer(GraphicsContext& context)
    : Visualizer(), m_painterpreter(std::make_unique<Painterpreter>(context)) {}

WidgetVisualizer::~WidgetVisualizer() = default;

void WidgetVisualizer::visualize(valid_ptr<Scene*> scene) const {
    WidgetScene* widget_scene = dynamic_cast<WidgetScene*>(raw_pointer(scene));
    if (!widget_scene) {
        NOTF_LOG_CRIT("Failed to visualize Scene \"{}\": WidgetVisualizer can only visualize WidgetScenes",
                      scene->get_name());
        return;
    }

    AnyNode::Iterator iterator(widget_scene->get_widget());
    AnyNodeHandle node;
    while (iterator.next(node)) {
        const WidgetHandle widget = handle_cast<WidgetHandle>(node);
        if (widget) { m_painterpreter->paint(widget); }
    }
}

NOTF_CLOSE_NAMESPACE
