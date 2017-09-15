#include "engine/engine_test04.hpp"

#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/vector3.hpp"
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

//projection and modelview matrices
Xform3f P  = Xform3f::identity();
Xform3f MV = Xform3f::identity();

struct VertexPos {
    constexpr static StaticString name = "vPos";
    using type                         = Vector4f;
    using kind                         = AttributeKind::Position;
};

struct VertexColor {
    constexpr static StaticString name = "vColor";
    using type                         = Vector4f;
    using kind                         = AttributeKind::Color;
};

struct VertexNormal {
    constexpr static StaticString name = "vNormal";
    using type                         = Vector4f;
    using kind                         = AttributeKind::Normal;
};

struct VertexTexCoord {
    constexpr static StaticString name = "vTexCoord";
    using type                         = Vector2f;
    using kind                         = AttributeKind::TexCoord;
};
}
static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

int test04_main(int /*argc*/, char* /*argv*/ [])
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

        ShaderPtr blinn_phong_shader = Shader::load(
            *context.get(), "Blinn-Phong",
            "/home/clemens/code/notf/res/shaders/blinn_phong.vert",
            "/home/clemens/code/notf/res/shaders/blinn_phong.frag");
        auto shader_scope = blinn_phong_shader->scope();
        UNUSED(shader_scope);

        // setup vertices
        using VertexLayout = VertexArray<VertexPos, VertexNormal>;
        VertexObject vertex_object(
            blinn_phong_shader,
            std::make_shared<VertexLayout>(GeometryFactory<VertexLayout>::produce()),
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

        glEnable(GL_DEPTH_TEST);

        // render loop
        float angle = 0;
        while (!glfwWindowShouldClose(window)) {
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
            //            glUniformMatrix4fv(blinn_phong_shader->uniform("modelview"), 1, GL_FALSE, modelview.as_ptr());

            const Xform3f perspective = Xform3f::perspective(deg_to_rad(160), 1, 0, 1000.f);
            blinn_phong_shader->set_uniform("projection", perspective);
            //            glUniformMatrix4fv(blinn_phong_shader->uniform("projection"), 1, GL_FALSE, perspective.as_ptr());

            //            const Xform3f perspective = Xform3f::orthographic(-400.f, 400.f, -400.f, 400.f, 0.f, 1000.f);
            //            glUniformMatrix4fv(blinn_phong_shader->uniform("projection"), 1, GL_FALSE, perspective.as_ptr());

            Xform3f normalMat = rotate;
            blinn_phong_shader->set_uniform("normalMat", normalMat);
            //            glUniformMatrix4fv(blinn_phong_shader->uniform("normalMat"), 1, GL_FALSE, normalMat.as_ptr());

            vertex_object.render();

            check_gl_error();

            glfwSwapBuffers(window);
            glfwPollEvents();
            angle += 0.01f;

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(16ms);
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
