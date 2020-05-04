#include "notf/app/widget/widget_visualizer.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/plotter/plotter.hpp"

#include "notf/app/graph/window.hpp"
#include "notf/app/widget/any_widget.hpp"
#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetVisualizer::WidgetVisualizer(const Window& window)
    : Visualizer(window), m_plotter(std::make_unique<Plotter>(window.get_graphics_context())) {}

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

    m_plotter->start_parsing();
    while (iterator.next(node)) {
        WidgetHandle widget = handle_cast<WidgetHandle>(node);
        if (widget) {
            m_plotter->parse(WidgetHandle::AccessFor<WidgetVisualizer>::get_design(widget), widget->get_xform());
        }
    }
    m_plotter->finish_parsing(); // TODO Painterpreter::Picture RAII instance?
}

NOTF_CLOSE_NAMESPACE
