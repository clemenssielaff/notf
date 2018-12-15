#include "notf/app/graph/scene.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/graphics_context.hpp"

#include "notf/app/graph/visualizer.hpp"
#include "notf/app/graph/window.hpp"

NOTF_OPEN_NAMESPACE

// scene ========================================================================================================== //

Scene::Scene(valid_ptr<AnyNode*> parent, valid_ptr<VisualizerPtr> visualizer)
    : super_t(parent), m_visualizer(std::move(visualizer)) {}

bool Scene::is_window_scene() const { return _has_parent_of_type<Window>(); }

void Scene::draw() {
    GraphicsContext& context = GraphicsContext::get();
    NOTF_ASSERT(context.is_current());

    // define render area
    if (is_window_scene()) {
        context.set_render_area(context.get_window_size());
    } else {
        const Aabri& my_area = get<area>();
        if (my_area.is_zero()) { return; }
        if (!my_area.is_valid()) {
            NOTF_LOG_WARN("Cannot draw a Layer with an invalid area");
            return;
        }
        context.set_render_area(my_area);
    }

    m_visualizer->visualize(this);
}

NOTF_CLOSE_NAMESPACE
