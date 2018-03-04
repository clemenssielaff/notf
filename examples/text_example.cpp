#include <iostream>

#include "app/core/glfw.hpp"
#include "app/renderer/plotter.hpp"
#include "app/scene/layer.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/text/font.hpp"

#pragma clang diagnostic ignored "-Wunused-variable"

NOTF_USING_NAMESPACE

namespace {

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

void render_thread(GLFWwindow* window)
{
    SceneManagerPtr manager = SceneManager::create(window);

    // Shader ///////////////////////////////////////////////

    PlotterPtr plotter = Plotter::create(manager);

    FontPtr font = Font::load(manager->font_manager(), "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 32);

    Plotter::TextInfo info;
    info.font        = font;
    info.translation = Vector2f{150, 100};

    auto what = "NoTF";
    plotter->add_text(info, what);

    plotter->apply();

    // Render State //////////////////////////////////////////////

    LayerPtr layer = Layer::create(manager, plotter);

    SceneManager::State state;
    state.layers = {layer};

    SceneManager::StateId state_id = manager->add_state(std::move(state));
    manager->enter_state(state_id);

    // Rendering //////////////////////////////////////////////

    manager->graphics_context()->clear(Color(0.2f, 0.3f, 0.5f, 1));

    while (!glfwWindowShouldClose(window)) {

        manager->render();

        glfwPollEvents();
    }
}

} // namespace

int text_main(int /*argc*/, char* /*argv*/ [])
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
