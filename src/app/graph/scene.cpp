#include "notf/app/graph/scene.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/graphics_context.hpp"

#include "notf/app/graph/visualizer.hpp"
#include "notf/app/graph/window.hpp"

NOTF_OPEN_NAMESPACE

// scene ========================================================================================================== //

Scene::Scene(valid_ptr<AnyNode*> parent) : super_t(parent) {}

Scene::~Scene() = default;

void Scene::set_visualizer(VisualizerPtr visualizer) { m_visualizer = std::move(visualizer); }

bool Scene::is_window_scene() const { return _has_parent_of_type<Window>(); }

void Scene::_draw() {
    if(!m_visualizer){
        NOTF_THROW(LogicError, "Cannot draw Scene \"{}\" without defining a visualizer first");
    }

    WindowHandle window = get_first_ancestor<Window>();
    if(!window){
        NOTF_THROW(GraphError, "Scene \"{}\" cannot be drawn because it is not child of a Window", get_name());
    }
    GraphicsContext& context = window->get_graphics_context();
    NOTF_ASSERT(context.is_current());

    // define render area
    if (is_window_scene()) {
        context->framebuffer.set_render_area(context->framebuffer.get_size());
    } else {
        const Aabri& my_area = get<area>();
        if (my_area.is_zero()) { return; }
        if (!my_area.is_valid()) {
            NOTF_LOG_WARN("Cannot draw a Layer with an invalid area");
            return;
        }
        context->framebuffer.set_render_area(my_area);
    }

    m_visualizer->visualize(this);
}

NOTF_CLOSE_NAMESPACE
