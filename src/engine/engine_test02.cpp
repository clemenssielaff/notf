#include "engine/engine_test02.hpp"

#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/vector3.hpp"
#include "common/xform3.hpp"
#include "core/glfw.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex_buffer.hpp"

using namespace notf;

namespace {

//vertex array and vertex buffer object IDs
GLuint vaoID;
GLuint vboIndicesID;

//triangle vertices and indices
GLushort indices[3];

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


void render_thread(GLFWwindow* window)
{
    std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

    ShaderPtr shader = Shader::load(
        *context.get(), "TestShader",
        "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.vert",
        "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.frag");
    context->push_shader(shader);

    //setup triangle vao
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    using VertexLayout = VertexBuffer<VertexPos, VertexColor>;
    std::vector<VertexLayout::Vertex> buffer_vertices;
    buffer_vertices.reserve(3);
    buffer_vertices.emplace_back(Vector3f(-1, -1, 0), Vector3f(1, 0, 0));
    buffer_vertices.emplace_back(Vector3f(0, 1, 0), Vector3f(0, 1, 0));
    buffer_vertices.emplace_back(Vector3f(1, -1, 0), Vector3f(0, 0, 1));

    //setup triangle indices
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    //pass indices to element array buffer
    glGenBuffers(1, &vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

    //setup triangle vao and vbo stuff

    VertexLayout vbuff(std::move(buffer_vertices));
    vbuff.init(shader);


    Size2i buffer_size;
    glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
    glViewport(0, 0, buffer_size.width, buffer_size.height);

    glClearColor(0, 0, 0, 1);

    const Xform3f identityXform = Xform3f::identity();;
    glUniformMatrix4fv(shader->uniform("MVP"), 1, GL_FALSE, identityXform.as_ptr());

    using namespace std::chrono_literals;
    auto lasttime = std::chrono::high_resolution_clock::now();

    // render loop
    float angle = 0;

    auto one_second = std::chrono::duration<double, std::milli>(1000);


    bool found_average = 0;
    bool started_average_counting = false;
    decltype (std::chrono::high_resolution_clock::now()) total_start;
    double average_counter = 0;
    double elapsed_average = 0;
    size_t total_frames = 0;


    size_t frame_count =0 ;
    while (!glfwWindowShouldClose(window)) {

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = now-lasttime;
        angle += 0.001 * elapsed.count();
        lasttime = now;

        if(!found_average){
            ++total_frames;
            if(!started_average_counting){
                if(total_frames > 10000){
                    started_average_counting = true;
                    total_frames = 0;
                    total_start = now;
                }
            } else {
                average_counter += elapsed.count();
                if(total_frames > 100000){
                    found_average = true;
                    elapsed_average = average_counter / static_cast<double>(100000);
                    log_info << "Found frame average at: " << elapsed_average << "ms";
                }
            }
        } else {
            if(elapsed.count() >= elapsed_average * 10){
                log_warning << "Frame drop detected, took " << elapsed.count() << "ms instead of " << elapsed_average << "ms";
            }
        }

        one_second -= elapsed;
        ++frame_count;
        if(one_second.count() <= 0){
            one_second = std::chrono::duration<double, std::milli>(1000);
            log_trace << frame_count;
            frame_count = 0;
        }


//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //pass the shader uniform
//        Xform3f xform = Xform3f::rotation(Vector4f(0, 0, 1, 1), angle);
//        glUniformMatrix4fv(shader->uniform("MVP"), 1, GL_FALSE, xform.as_ptr());
//        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);

        glfwSwapBuffers(window);
    }

    context->clear_shader();

    // destroy vao and vbo
    glDeleteBuffers(1, &vboIndicesID);
    glDeleteVertexArrays(1, &vaoID);

}

int test02_main(int /*argc*/, char* /*argv*/ [])
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

    log_info << "TEST 02";

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
