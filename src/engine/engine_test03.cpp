#include "engine/engine_test03.hpp"

#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/vector3.hpp"
#include "common/xform3.hpp"
#include "core/glfw.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/index_buffer.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex_array.hpp"
#include "graphics/vertex_object.hpp"

using namespace notf;

namespace {

//projection and modelview matrices
Xform3f P  = Xform3f::identity();
Xform3f MV = Xform3f::identity();

struct VertexPos {
    constexpr static StaticString name = "vVertex";
    using type                         = Vector3f;
};

struct VertexColor {
    constexpr static StaticString name = "vColor";
    using type                         = Vector3f;
};
}
static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

int test03_main(int /*argc*/, char* /*argv*/ [])
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
        GLFWwindow* window = glfwCreateWindow(800, 800, "NoTF Engine Test", nullptr, nullptr);
        std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

        ShaderPtr shader = Shader::load(
            *context.get(), "TestShader",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.vert",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.frag");
        context->push_shader(shader);

        // setup vertices
        using VertexLayout = VertexArray<VertexPos, VertexColor>;
        std::vector<VertexLayout::Vertex> buffer_vertices;
        buffer_vertices.reserve(3);
        buffer_vertices.emplace_back(Vector3f(-1, -1, 0), Vector3f(1, 0, 0));
        buffer_vertices.emplace_back(Vector3f(0, 1, 0), Vector3f(0, 1, 0));
        buffer_vertices.emplace_back(Vector3f(1, -1, 0), Vector3f(0, 0, 1));

        VertexObject vertex_object(
            shader,
            std::make_shared<VertexLayout>(std::move(buffer_vertices)),
            VertexObject::RenderMode::TRIANGLES,
            create_index_buffer<0, 1, 2>());

        // render loop
        float angle = 0;
        while (!glfwWindowShouldClose(window)) {
            Size2i buffer_size;
            glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
            glViewport(0, 0, buffer_size.width, buffer_size.height);

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            //pass the shader uniform
//            Xform3f xform = Xform3f::orthographic(0, static_cast<float>(buffer_size.width),
//                                                  0, static_cast<float>(buffer_size.height), -1, 1)
//                * Xform3f::translation(buffer_size.width / 2.f, buffer_size.height / 2.f)
//                * Xform3f::rotation(Vector4f(0, 0, 1, 1), angle)
//                * Xform3f::scaling(100);
//            glUniformMatrix4fv(shader->uniform("MVP"), 1, GL_FALSE, xform.as_ptr());

            vertex_object.render();

            check_gl_error();

            glfwSwapBuffers(window);
            glfwPollEvents();
            angle += 0.0001f;
        }

        context->clear_shader();
    }

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
