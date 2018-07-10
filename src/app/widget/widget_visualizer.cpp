#include "app/widget/widget_visualizer.hpp"

#include "app/window.hpp"
#include "graphics/renderer/plotter.hpp"

NOTF_OPEN_NAMESPACE

WidgetVisualizer::WidgetVisualizer(Window& window)
    : Visualizer(), m_renderer(std::make_unique<Plotter>(window.get_graphics_context()))
{}

WidgetVisualizer::~WidgetVisualizer() = default;

void WidgetVisualizer::visualize(valid_ptr<Scene*> scene) const
{
    // TODO: WidgetVisualizer::_visualize
}

NOTF_CLOSE_NAMESPACE
