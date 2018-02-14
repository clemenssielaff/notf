#include <iostream>

#include "app/core/glfw.hpp"
#include "app/graphics/plotter.hpp"
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

    Plotter plotter(graphics_context);

    Polygonf polygon({Vector2f{100, 700}, Vector2f{50, 200}, Vector2f{50, 50}, Vector2f{750, 50}, Vector2f{750, 750}});
    //    Polygonf polygon({
    //        Vector2f{565, 770},
    //        Vector2f{040, 440},
    //        Vector2f{330, 310},
    //        Vector2f{150, 120},
    //        Vector2f{460, 230},
    //        Vector2f{770, 120},
    //        Vector2f{250, 450},
    //    });

    Plotter::ShapeInfo info;

    plotter.add_shape(info, polygon);

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

        // perform polygon rendering //////////////////////////////////////////

        //        gl_check(glEnable(GL_STENCIL_TEST)); // enable stencil
        //        gl_check(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)); // do not write into color buffer
        //        gl_check(glStencilMask(0xff)); // write to all bits of the stencil buffer
        //        gl_check(glStencilFunc(GL_ALWAYS, 0, 1)); //  Always pass (other values are default values and do not
        //        matter for GL_ALWAYS)

        //        gl_check(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP));
        //        gl_check(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP));
        //        gl_check(glDisable(GL_CULL_FACE));
        //        gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr));
        //        gl_check(glEnable(GL_CULL_FACE));

        //        gl_check(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
        //        gl_check(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff)); // only write to pixels that are inside the polygon
        //        gl_check(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than
        //        clearing it at the start)

        // render colors here, same area as before if you don't want to clear the stencil buffer every time
        //        gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices->size()), GL_UNSIGNED_INT, nullptr));

        //        gl_check(glDisable(GL_STENCIL_TEST));

        plotter.render();

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
