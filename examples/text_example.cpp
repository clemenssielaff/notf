#include <iostream>

#include "app/core/glfw.hpp"
#include "common/half.hpp"
#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/polygon.hpp"
#include "common/size2.hpp"
#include "common/system.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"
#include "common/warnings.hpp"
#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/prefab_factory.hpp"
#include "graphics/core/prefab_group.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/core/vertex_array.hpp"
#include "graphics/engine/plotter.hpp"
#include "graphics/text/font.hpp"

#include "glm_utils.hpp"

using namespace notf;

namespace {

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

void render_thread(GLFWwindow* window)
{
    std::unique_ptr<GraphicsContext> graphics_context(new GraphicsContext(window));

    // Shader ///////////////////////////////////////////////

    FontManager& font_manager = graphics_context->font_manager();
    FontPtr font = Font::load(*graphics_context, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 28);

    Plotter plotter(graphics_context);

    Plotter::TextInfo info;
    info.font        = font;
    info.translation = Vector2f{100, 100};

    plotter.add_text(info, "abp");

    plotter.apply();

    // Rendering //////////////////////////////////////////////

    // render loop
    using namespace std::chrono_literals;
    auto last_frame_start_time = std::chrono::high_resolution_clock::now();
    size_t frame_counter       = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        if (frame_start_time - last_frame_start_time > 1s) {
            last_frame_start_time = frame_start_time;
            log_info << frame_counter << "fps";
            frame_counter = 0;
        }
        ++frame_counter;

        Size2i buffer_size;
        glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
        glViewport(0, 0, buffer_size.width, buffer_size.height);

        glClearColor(0.2f, 0.3f, 0.5f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        plotter.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // clean up
    graphics_context->unbind_all_textures();
    graphics_context->unbind_framebuffer();
    graphics_context->unbind_pipeline();
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
