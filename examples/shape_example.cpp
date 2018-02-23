#include <iostream>

#include "app/core/glfw.hpp"
#include "common/log.hpp"
#include "common/polygon.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/vertex_array.hpp"
#include "graphics/engine/layer.hpp"
#include "graphics/producer/plotter.hpp"

#pragma clang diagnostic ignored "-Wunused-variable"

using namespace notf;

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
    RenderManagerPtr render_manager = RenderManager::create(window);

    // Shader ///////////////////////////////////////////////

    PlotterPtr plotter = Plotter::create(render_manager);

#if 0
    Polygonf polygon({Vector2f{100, 700}, Vector2f{50, 200}, Vector2f{50, 50}, Vector2f{750, 50}, Vector2f{750, 750}});
#else
    Polygonf polygon({
        Vector2f{565, 770},
        Vector2f{040, 440},
        Vector2f{330, 310},
        Vector2f{150, 120},
        Vector2f{460, 230},
        Vector2f{770, 120},
        Vector2f{250, 450},
    });
#endif
    Plotter::ShapeInfo info;

    plotter->add_shape(info, polygon);

    plotter->apply();

    // Render State //////////////////////////////////////////////

    LayerPtr layer = Layer::create(render_manager, plotter);

    RenderManager::State state;
    state.layers = {layer};

    RenderManager::StateId state_id = render_manager->add_state(std::move(state));
    render_manager->enter_state(state_id);

    // Rendering //////////////////////////////////////////////

    render_manager->graphics_context()->clear(Color(0.2f, 0.3f, 0.5f, 1));

    while (!glfwWindowShouldClose(window)) {

        render_manager->render();

        glfwPollEvents();
    }
}

} // namespace

int shape_main(int /*argc*/, char* /*argv*/ [])
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
