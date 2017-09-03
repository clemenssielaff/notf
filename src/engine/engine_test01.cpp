#include "engine/engine_test01.hpp"

#include "common/log.hpp"
#include "common/size2.hpp"
#include "core/glfw.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/shader.hpp"

using namespace notf;

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

int test01_main(int /*argc*/, char* /*argv*/ [])
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
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    { // open the window
        GLFWwindow* window = glfwCreateWindow(800, 600, "NoTF Engine Test", nullptr, nullptr);
        std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

        ShaderPtr shader = Shader::load(
            *context.get(), "TestShader",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.vert",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.frag");

        // render loop
        while (!glfwWindowShouldClose(window)) {
            Size2i buffer_size;
            glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
            glViewport(0, 0, buffer_size.width, buffer_size.height);

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
