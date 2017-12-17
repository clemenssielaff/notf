#include <iostream>

#include "app/core/glfw.hpp"
#include "common/half.hpp"
#include "common/log.hpp"
#include "common/matrix4.hpp"
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

#include "glm_utils.hpp"

using namespace notf;

namespace {

struct VertexPos : public AttributeTrait {
    constexpr static uint location = 0;
    using type                     = Vector2f;
    using kind                     = AttributeKind::Position;
};

struct InstanceXform : public AttributeTrait {
    constexpr static uint location = 3;
    using type                     = Matrix4f;
    using kind                     = AttributeKind::Other;
};

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

struct Foo {
    int a, b, c;
};

void render_thread(GLFWwindow* window)
{
    std::unique_ptr<GraphicsContext> graphics_context(new GraphicsContext(window));

    // Shader ///////////////////////////////////////////////

    const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/trivial.vert");
    VertexShaderPtr vertex_shader = VertexShader::build(graphics_context, "trivial.vert", vertex_src);

    const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/trivial.frag");
    FragmentShaderPtr frag_shader = FragmentShader::build(graphics_context, "trivial.frag", frag_src);

    PipelinePtr pipeline = Pipeline::create(graphics_context, vertex_shader, frag_shader);
    graphics_context->bind_pipeline(pipeline);

    // Vertices ///////////////////////////////////////////////

    GLuint vao;
    gl_check(glGenVertexArrays(1, &vao));
    gl_check(glBindVertexArray(vao));

#if 0

    auto vertices = std::make_unique<VertexArray<VertexPos>>();
    vertices->init();
    vertices->update({
        Vector2f{565, 800 - 30},
        Vector2f{40, 800 - 360},
        Vector2f{330, 800 - 490},
        Vector2f{150, 800 - 680},
        Vector2f{460, 800 - 570},
        Vector2f{770, 800 - 680},
        Vector2f{250, 800 - 350},
    });

    auto indices = std::make_unique<IndexArray<GLuint>>();
    indices->init();
    indices->update({
        0, 0, 1, 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 0,
    });

#else

        auto vertices = std::make_unique<VertexArray<VertexPos>>();
        vertices->init();
        vertices->update({Vector2f{50 , 750},
                          Vector2f{50 , 50},
                          Vector2f{750, 50},
                          Vector2f{750, 750},
                          Vector2f{250, 550},
                          Vector2f{250, 250},
                          Vector2f{550, 250},
                          Vector2f{550, 550},
                         });

        auto indices = std::make_unique<IndexArray<GLuint>>();
        indices->init();
        indices->update({0, 0, 1,
                         0, 1, 2,
                         0, 2, 3,
                         0, 3, 0,

#if 1 // CW = hole
                         0, 5, 4,
                         0, 6, 5,
                         0, 7, 6,
                         0, 4, 7,
#else // CCW = fill
                         0, 4, 5,
                         0, 5, 6,
                         0, 6, 7,
                         0, 7, 4,
#endif
                        });

#endif

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

        {
            // pass the shader uniforms
            const Matrix4f perspective = Matrix4f::orthographic(0.f, 800.f, 0.f, 800.f, 0.f, 10000.f);
            vertex_shader->set_uniform("projection", perspective);

            const Matrix4f move      = Matrix4f::translation(0, 0, -500);
            const Matrix4f rotate    = Matrix4f::identity();
            const Matrix4f scale     = Matrix4f::scaling(1);
            const Matrix4f modelview = move * rotate * scale;
            vertex_shader->set_uniform("modelview", modelview);
        }

        glClearColor(0.2f, 0.3f, 0.5f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // perform polygon rendering //////////////////////////////////////////

        glEnable(GL_CULL_FACE);

        glEnable(GL_STENCIL_TEST); // enable stencil
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // do not write into color buffer
        glStencilMask(0xff); // write to all bits of the stencil buffer
        glStencilFunc(GL_ALWAYS, 0, 1); //  Always pass (other values are default values and do not matter for GL_ALWAYS)

        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr);
        glEnable(GL_CULL_FACE);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // re-enable color
        glStencilFunc(GL_NOTEQUAL, 0x00, 0xff); // only write to pixels that are inside the polygon
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO); // reset the stencil buffer (is a lot faster than clearing it at the start)

        // render colors here, same area as before if you don't want to clear the stencil buffer every time
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr);

        glDisable(GL_STENCIL_TEST);

        ///////////////////////////////////////////////////////////////////////

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // clean up
    graphics_context->unbind_all_textures();
    graphics_context->unbind_framebuffer();
    graphics_context->unbind_pipeline();
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
