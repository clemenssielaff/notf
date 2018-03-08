#include "app/core/glfw.hpp"
#include "app/render/plotter.hpp"
#include "app/scene/layer.hpp"
#include "app/scene/widget/hierarchy.hpp"
#include "app/scene/scene_manager.hpp"
#include "common/bezier.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/vertex_array.hpp"

#pragma clang diagnostic ignored "-Wunused-variable"

NOTF_USING_NAMESPACE

namespace {

struct VertexPos : public AttributeTrait {
    constexpr static uint location = 0;
    using type                     = Vector2f;
    using kind                     = AttributeKind::Position;
};

struct LeftCtrlPos : public AttributeTrait {
    constexpr static uint location = 1;
    using type                     = Vector2f;
};

struct RightCtrlPos : public AttributeTrait {
    constexpr static uint location = 2;
    using type                     = Vector2f;
};

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

void render_thread(GLFWwindow* window)
{
    SceneManagerPtr scene_manager = SceneManager::create(window);

    // Stroker ////////////////////////////////////////////////

    PlotterPtr stroker = Plotter::create(scene_manager);

    {
        Plotter::StrokeInfo stroke_info;
        stroke_info.width = 1.0;

        CubicBezier2f spline1({
            CubicBezier2f::Segment(Vector2f{100, 200}, Vector2f{400, 100}, Vector2f{400, 700}, Vector2f{700, 700}),
        });
        stroker->add_stroke(stroke_info, spline1);
    }

    {
        Plotter::StrokeInfo stroke_info;
        stroke_info.width = 3.0;

        CubicBezier2f spline2({
            CubicBezier2f::Segment::line(Vector2f{100, 100}, Vector2f{200, 150}),
            CubicBezier2f::Segment::line(Vector2f{200, 150}, Vector2f{300, 100}),
            CubicBezier2f::Segment::line(Vector2f{300, 100}, Vector2f{400, 200}),
        });

        stroker->add_stroke(stroke_info, spline2);
    }

    stroker->apply();

    // Render State //////////////////////////////////////////////

    ItemHierarchyPtr scene = ItemHierarchy::create();
    LayerPtr layer = Layer::create(scene_manager, scene, stroker);

    SceneManager::State state;
    state.layers = {layer};

    SceneManager::StateId state_id = scene_manager->add_state(std::move(state));
    scene_manager->enter_state(state_id);

    // Rendering //////////////////////////////////////////////

    scene_manager->graphics_context()->clear(Color(0.2f, 0.3f, 0.5f, 1));

    while (!glfwWindowShouldClose(window)) {

        scene_manager->request_redraw();

        glfwPollEvents();
    }
}

} // namespace

int line_main(int /*argc*/, char* /*argv*/ [])
{
    // install the log handler first, to catch errors right away
    auto log_handler = std::make_unique<LogHandler>(128, 200);
    install_log_message_handler(std::bind(&LogHandler::push_log, log_handler.get(), std::placeholders::_1));
    log_handler->start();
    glfwSetErrorCallback(error_callback);

    // initialize GLFW
    if (!glfwInit()) {
        log_fatal << "GLFW initialization failed";
        exit(-1);
    }
    log_info << "GLFW version: " << glfwGetVersionString();

    // NoTF uses OpenGL ES 3.2
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    { // open the window
        GLFWwindow* window = glfwCreateWindow(800, 800, "NoTF Engine Test", nullptr, nullptr);
        std::thread render_worker(render_thread, window);
        while (!glfwWindowShouldClose(window)) {
            glfwWaitEvents();
        }
        render_worker.join();
        glfwDestroyWindow(window);
    }

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
