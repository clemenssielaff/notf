#include "engine/engine_test.hpp"

#include <iostream>

#include "common/half.hpp"
#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"
#include "common/warnings.hpp"
#include "common/xform3.hpp"
#include "core/glfw.hpp"
#include "graphics/geometry.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/index_buffer.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex_array.hpp"
#include "graphics/vertex_object.hpp"

#include "glm_utils.hpp"

using namespace notf;

namespace {

struct VertexPos {
    StaticString name             = "vPos";
    using type                    = float;
    constexpr static size_t count = 4;
    using kind                    = AttributeKind::Position;
};

struct VertexColor {
    StaticString name             = "vColor";
    using type                    = float;
    constexpr static size_t count = 4;
    using kind                    = AttributeKind::Color;
};

struct VertexNormal {
    StaticString name             = "vNormal";
    using type                    = float;
    constexpr static size_t count = 4;
    using kind                    = AttributeKind::Normal;
};

struct VertexTexCoord {
    StaticString name             = "vTexCoord";
    using type                    = float;
    constexpr static size_t count = 2;
    using kind                    = AttributeKind::TexCoord;
};
}
static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

void render_thread(GLFWwindow* window)
{
    std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

    ShaderPtr blinn_phong_shader = Shader::load(
        *context.get(), "Blinn-Phong",
        "/home/clemens/code/notf/res/shaders/blinn_phong.vert",
        "/home/clemens/code/notf/res/shaders/blinn_phong.frag");
    auto shader_scope = blinn_phong_shader->scope();
    UNUSED(shader_scope);

    // setup vertices
    using VertexLayout = VertexArray<VertexPos, VertexNormal>;
    auto vertex_buffer = std::make_shared<VertexLayout>(GeometryFactory<VertexLayout>::produce());
    VertexObject vertex_object(
        blinn_phong_shader,
        vertex_buffer,
        VertexObject::RenderMode::TRIANGLES,
        create_index_buffer<0, 2, 1,
                            0, 3, 2,
                            4, 6, 5,
                            4, 7, 6,
                            8, 10, 11,
                            8, 9, 10,
                            12, 14, 15,
                            12, 13, 14,
                            16, 18, 19,
                            16, 17, 18,
                            23, 21, 20,
                            23, 22, 21>());

    {
        size_t i = 0;
        for (const VertexLayout::Vertex& vertex : vertex_buffer->m_vertices) {
            auto first = std::get<0>(vertex);
            auto second = std::get<1>(vertex);
            std::stringstream ss;
            ss << i++ << ": ( ";

            for(size_t j =0; j < 3; ++j){
                ss << first[j] << ", ";
            }
            ss << first[3] << " ), ( ";

            for(size_t j =0; j < 3; ++j){
                ss << second[j] << ", ";
            }
            ss << second[3] << " )";

            log_trace << ss.str();
        }
    }

    glEnable(GL_DEPTH_TEST);

    // render loop
    using namespace std::chrono_literals;
    auto last_frame_start_time = std::chrono::high_resolution_clock::now();
    float angle                = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        angle += 0.01 * ((frame_start_time - last_frame_start_time) / 16ms);
        last_frame_start_time = frame_start_time;

        Size2i buffer_size;
        glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
        glViewport(0, 0, buffer_size.width, buffer_size.height);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // pass the shader uniforms
        const Xform3f move      = Xform3f::translation(0, 0, -500);
        const Xform3f rotate    = Xform3f::rotation(Vector4f(sin(angle), cos(angle)), angle);
        const Xform3f scale     = Xform3f::scaling(100);
        const Xform3f modelview = move * rotate * scale;
        blinn_phong_shader->set_uniform("modelview", modelview);

        const Xform3f perspective = Xform3f::perspective(deg_to_rad(90), 1, 0, 1000.f);
        // const Xform3f perspective = Xform3f::orthographic(-400.f, 400.f, -400.f, 400.f, 0.f, 1000.f);
        blinn_phong_shader->set_uniform("projection", perspective);

        Xform3f normalMat = rotate;
        blinn_phong_shader->set_uniform("normalMat", normalMat);

        vertex_object.render();

        check_gl_error();

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto sleep_time = max(0ms, 16ms - (std::chrono::high_resolution_clock::now() - frame_start_time));
        std::this_thread::sleep_for(sleep_time);
    }

    context->clear_shader();
}

int test_main(int /*argc*/, char* /*argv*/ [])
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
        std::thread render_worker(render_thread, window);
        while (!glfwWindowShouldClose(window)) {
            glfwWaitEvents();
        }
        render_worker.join();
    }

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
