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
    std::unique_ptr<GraphicsContext> graphics_context(new GraphicsContext(window));

    // Shader ///////////////////////////////////////////////

    const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/shape.vert");
    VertexShaderPtr vertex_shader = VertexShader::build(graphics_context, "shape.vert", vertex_src);

    const std::string tess_src       = load_file("/home/clemens/code/notf/res/shaders/shape.tess");
    const std::string eval_src       = load_file("/home/clemens/code/notf/res/shaders/shape.eval");
    TesselationShaderPtr tess_shader = TesselationShader::build(graphics_context, "shape.tess", tess_src, eval_src);

    const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/shape.frag");
    FragmentShaderPtr frag_shader = FragmentShader::build(graphics_context, "shape.frag", frag_src);

    PipelinePtr pipeline = Pipeline::create(graphics_context, vertex_shader, tess_shader, frag_shader);
    graphics_context->bind_pipeline(pipeline);

    // Vertices ///////////////////////////////////////////////

    GLuint vao;
    gl_check(glGenVertexArrays(1, &vao));
    gl_check(glBindVertexArray(vao));

#if 0

    auto vertices = std::make_unique<VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>>();
    vertices->init();
    vertices->update({
        { Vector2f{565, 770}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{ 40, 440}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{330, 310}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{150, 120}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{460, 230}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{770, 120}, Vector2f{0, 0}, Vector2f{0, 0} },
        { Vector2f{250, 450}, Vector2f{0, 0}, Vector2f{0, 0} },
    });

    auto indices = std::make_unique<IndexArray<GLuint>>();
    indices->init();
    indices->update({
        0, 1,
        1, 2,
        2, 3,
        3, 4,
        4, 5,
        5, 6,
        6, 0,
    });

#else

        auto vertices = std::make_unique<VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>>();
        vertices->init();
        vertices->update({{ Vector2f{100 , 700}, Vector2f{100, 0}, Vector2f{0, 0} },
                          { Vector2f{50 , 50 }, Vector2f{0, 0}, Vector2f{0, 0} },
                          { Vector2f{750, 50 }, Vector2f{0, 0}, Vector2f{0, 0} },
                          { Vector2f{750, 750}, Vector2f{0, 0}, Vector2f{0, 0} },

                          { Vector2f{250, 550}, Vector2f{0, 0}, Vector2f{0, 0} },
                          { Vector2f{250, 250}, Vector2f{0, 0}, Vector2f{0, 0} },
                          { Vector2f{550, 250}, Vector2f{0, 0}, Vector2f{0, 0} },
                          { Vector2f{550, 550}, Vector2f{0, 0}, Vector2f{0, 0} },
                         });

        auto indices = std::make_unique<IndexArray<GLuint>>();
        indices->init();
        indices->update({
                         0, 1,
                         1, 2,
                         2, 3,
                         3, 0,
#if 0
#if 1 // CW = hole
                         5, 4,
                         6, 5,
                         7, 6,
                         4, 7,
#else // CCW = fill
                         4, 5,
                         5, 6,
                         6, 7,
                         7, 4,
#endif
#endif
                        });

#endif

    log_info << tess_shader->control_source();

    // Rendering //////////////////////////////////////////////

    gl_check(glEnable(GL_BLEND));
    gl_check(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    gl_check(glEnable(GL_CULL_FACE));
    gl_check(glPatchParameteri(GL_PATCH_VERTICES, 2));

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
            tess_shader->set_uniform("projection", perspective);

            // with a purely convex polygon, we can safely put the base vertex into the center of the polygon as it will
            // always be inside and it should never fall onto an existing polygon
            tess_shader->set_uniform("base_vertex", Vector2f{400 , 400});

            tess_shader->set_uniform("aa_width", 1.2f);

            // I can only be sure that the polygon is convex, if the polygon containing the control points as well
            // is convex - otherwise a convex polygon can become concave through the bezierness
            tess_shader->set_uniform("patch_type", 1);
        }

        glClearColor(0.2f, 0.3f, 0.5f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // perform polygon rendering //////////////////////////////////////////

//        gl_check(glEnable(GL_STENCIL_TEST)); // enable stencil
//        gl_check(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)); // do not write into color buffer
//        gl_check(glStencilMask(0xff)); // write to all bits of the stencil buffer
//        gl_check(glStencilFunc(GL_ALWAYS, 0, 1)); //  Always pass (other values are default values and do not matter for GL_ALWAYS)

//        gl_check(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP));
//        gl_check(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP));
//        gl_check(glDisable(GL_CULL_FACE));
//        gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr));
//        gl_check(glEnable(GL_CULL_FACE));

//        gl_check(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
//        gl_check(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff)); // only write to pixels that are inside the polygon
//        gl_check(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than clearing it at the start)

        // render colors here, same area as before if you don't want to clear the stencil buffer every time
        gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr));

//        gl_check(glDisable(GL_STENCIL_TEST));

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
