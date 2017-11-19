#include <iostream>

#include "common/half.hpp"
#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/size2.hpp"
#include "common/system.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"
#include "common/warnings.hpp"
#include "core/glfw.hpp"
#include "graphics/frame_buffer.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/prefab_factory.hpp"
#include "graphics/prefab_group.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/vertex_array.hpp"

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

    const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/line.vert");
    VertexShaderPtr vertex_shader = VertexShader::build(graphics_context, "Line.vert", vertex_src.c_str());

    const std::string tess_src = load_file("/home/clemens/code/notf/res/shaders/line.tess");
    const std::string eval_src = load_file("/home/clemens/code/notf/res/shaders/line.eval");
    TesselationShaderPtr tess_shader
        = TesselationShader::build(graphics_context, "Line.tess", tess_src.c_str(), eval_src.c_str());

#if 1
    const std::string geom_src   = load_file("/home/clemens/code/notf/res/shaders/line.geo");
    GeometryShaderPtr geo_shader = GeometryShader::build(graphics_context, "Line.geo", geom_src.c_str());

    const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/line.frag");
#else
    GeometryShaderPtr geo_shader = {};

    const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/line_no_wf.frag");
#endif
    FragmentShaderPtr frag_shader = FragmentShader::build(graphics_context, "Line.frag", frag_src.c_str());

    PipelinePtr pipeline = Pipeline::create(graphics_context, vertex_shader, tess_shader, geo_shader, frag_shader);
    graphics_context->bind_pipeline(pipeline);

    // Vertices ///////////////////////////////////////////////

    GLuint vao;
    gl_check(glGenVertexArrays(1, &vao));
    gl_check(glBindVertexArray(vao));

    auto vertices = std::make_unique<VertexArray<VertexPos>>();
    vertices->init();
    //    vertices->update({Vector2f{50, 50}, Vector2f{100, 750}, Vector2f{150, 50}, Vector2f{200, 750}});
    vertices->update({Vector2f{50, 50}, Vector2f{750, 50}, Vector2f{750, 750}, Vector2f{50, 750}});

        auto indices       = std::make_unique<IndexArray<GLuint>>();
        indices->m_indices = {0, 1, 2, 0, 2, 3};
        indices->init();

    // Rendering //////////////////////////////////////////////

    //        glEnable(GL_CULL_FACE);
    //        glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render loop
    using namespace std::chrono_literals;
    auto last_frame_start_time = std::chrono::high_resolution_clock::now();
    size_t frame_counter = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        //last_frame_start_time = frame_start_time;
        if(frame_start_time - last_frame_start_time > 1s){
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

        {
            // pass the shader uniforms
            const Matrix4f perspective = Matrix4f::orthographic(0.f, 800.f, 0.f, 800.f, 0.f, 10000.f);
            tess_shader->set_uniform("projection", perspective);

            // TODO: make sure that GL_PATCH_VERTICES <= GL_MAX_PATCH_VERTICES
            gl_check(glPatchParameteri(GL_PATCH_VERTICES, 3));

            glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices->m_indices.size()), GL_UNSIGNED_INT, nullptr);
//            gl_check(glDrawArrays(GL_PATCHES, 0, 3));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

//        const auto sleep_time = max(0ms, 16ms - (std::chrono::high_resolution_clock::now() - frame_start_time));
//        std::this_thread::sleep_for(sleep_time);
    }

    // clean up
    graphics_context->unbind_all_textures();
    graphics_context->unbind_framebuffer();
    graphics_context->unbind_pipeline();
}

} // namespace

int wireframe_main(int /*argc*/, char* /*argv*/ [])
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
