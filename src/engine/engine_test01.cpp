#include "engine/engine_test01.hpp"

#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/vector3.hpp"
#include "common/xform3.hpp"
#include "core/glfw.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/shader.hpp"

using namespace notf;

//out vertex struct for interleaved attributes
struct Vertex {
    Vector3f position;
    Vector3f color;
};

//vertex array and vertex buffer object IDs
GLuint vaoID;
GLuint vboVerticesID;
GLuint vboIndicesID;

//triangle vertices and indices
Vertex vertices[3];
GLushort indices[3];

//projection and modelview matrices
Xform3f P  = Xform3f::identity();
Xform3f MV = Xform3f::identity();

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
        GLFWwindow* window = glfwCreateWindow(800, 800, "NoTF Engine Test", nullptr, nullptr);
        std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

        ShaderPtr shader = Shader::load(
            *context.get(), "TestShader",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.vert",
            "/home/clemens/tutorial/OpenGL-Build-High-Performance-Graphics/Module 1/Chapter01/SimpleTriangle/SimpleTriangle/shaders/shader.frag");
        context->push_shader(shader);

        //setup triangle vertices
        vertices[0].position = Vector3f(-1, -1, 0);
        vertices[1].position = Vector3f(0, 1, 0);
        vertices[2].position = Vector3f(1, -1, 0);

        vertices[0].color = Vector3f(1, 0, 0);
        vertices[1].color = Vector3f(0, 1, 0);
        vertices[2].color = Vector3f(0, 0, 1);

        //setup triangle indices
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;

        //setup triangle vao and vbo stuff
        glGenVertexArrays(1, &vaoID);
        glGenBuffers(1, &vboVerticesID);
        glGenBuffers(1, &vboIndicesID);
        const GLsizei stride = sizeof(Vertex);

        glBindVertexArray(vaoID);

        //pass triangle verteices to buffer object
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        //enable vertex attribute array for position
        glEnableVertexAttribArray(shader->attribute("vVertex"));
        glVertexAttribPointer(shader->attribute("vVertex"), 3, GL_FLOAT, GL_FALSE, stride, 0);

        //enable vertex attribute array for colour
        glEnableVertexAttribArray(shader->attribute("vColor"));
        glVertexAttribPointer(shader->attribute("vColor"), 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*)offsetof(Vertex, color));

        //pass indices to element array buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

        // render loop
        float angle = 0;
        while (!glfwWindowShouldClose(window)) {
            Size2i buffer_size;
            glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
            glViewport(0, 0, buffer_size.width, buffer_size.height);

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            //pass the shader uniform
            Xform3f xform = Xform3f::rotation(Vector4f(0, 0, 1, 1), angle);
            glUniformMatrix4fv(shader->uniform("MVP"), 1, GL_FALSE, xform.as_ptr());
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);

            check_gl_error();

            glfwSwapBuffers(window);
            glfwPollEvents();
            angle += 0.0001f;
        }

        context->clear_shader();
    }

    // destroy vao and vbo
    glDeleteBuffers(1, &vboVerticesID);
    glDeleteBuffers(1, &vboIndicesID);
    glDeleteVertexArrays(1, &vaoID);

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
