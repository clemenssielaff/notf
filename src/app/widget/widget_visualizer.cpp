#include "notf/app/widget/widget_visualizer.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/widget/painterpreter.hpp"
#include "notf/app/widget/widget.hpp"
#include "notf/app/widget/widget_scene.hpp"
#include "notf/app/window.hpp"

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

    Node::Iterator iterator(widget_scene->get_widget());
    NodeHandle widget;
    while (true) {
        bool has_more = iterator.next(widget);
        m_painterpreter->paint(*widget);
        if (!has_more) { break; }
    }
}

NOTF_CLOSE_NAMESPACE
