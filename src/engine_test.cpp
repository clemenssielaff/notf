#include <iostream>

#include "common/half.hpp"
#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/warnings.hpp"
#include "core/glfw.hpp"
#include "graphics/engine/graphics_context.hpp"
#include "graphics/engine/prefab_factory.hpp"
#include "graphics/engine/prefab_group.hpp"
#include "graphics/engine/shader.hpp"
#include "graphics/engine/texture2.hpp"
#include "graphics/engine/vertex_array.hpp"

#include "glm_utils.hpp"

using namespace notf;

namespace {

struct VertexPos {
    constexpr static auto name    = "a_position";
    using type                    = float;
    constexpr static size_t count = 4;
    using kind                    = AttributeKind::Position;
};

struct VertexNormal {
    constexpr static auto name    = "a_normal";
    using type                    = float;
    constexpr static size_t count = 4;
    using kind                    = AttributeKind::Normal;
};

struct VertexTexCoord {
    constexpr static auto name    = "a_texcoord";
    using type                    = float;
    constexpr static size_t count = 2;
    using kind                    = AttributeKind::TexCoord;
};

struct InstanceXform {
    constexpr static auto name    = "i_xform";
    using type                    = float;
    constexpr static size_t count = 16;
    using kind                    = AttributeKind::Other;
};
}

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

void render_thread(GLFWwindow* window)
{
    std::unique_ptr<GraphicsContext> context(new GraphicsContext(window));

    //////////////////////////////////

    // check if GL_MAX_RENDERBUFFER_SIZE is >= texWidth and texHeight
    GLint texWidth = 800, texHeight = 800;
    GLint maxRenderbufferSize;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    if ((maxRenderbufferSize <= texWidth) || (maxRenderbufferSize <= texHeight)) {
        // cannot use framebuffer objects, as we need to create
        // a depth buffer as a renderbuffer object
        // return with appropriate error
        throw_runtime_error("Render target too large");
    }

    // bind the framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // bind texture and load the texture mip level 0
    // texels are RGB565
    // no texels need to be specified as we are going to draw into the texture
    GLuint renderTexture;
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // bind renderbuffer and create a 16-bit depth buffer
    // width and height of renderbuffer = width and height of
    // the texture
    GLuint depthRenderbuffer;
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, texWidth, texHeight);

    // specify texture as color attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);

    // specify depth_renderbuffer as depth attachment
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    // check for framebuffer complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw_runtime_error("UNDERBNESS");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /////////////////////////////////////

    ShaderPtr blinn_phong_shader = Shader::load(
        *context.get(), "Blinn-Phong",
        "/home/clemens/code/notf/res/shaders/blinn_phong.vert",
        "/home/clemens/code/notf/res/shaders/blinn_phong.frag");
    auto shader_scope = blinn_phong_shader->scope();
    UNUSED(shader_scope);

    Texture2::Args tex_args;
    tex_args.codec      = Texture2::Codec::ASTC;
    tex_args.anisotropy = 5;
    Texture2Ptr texture = Texture2::load_image(*context.get(), "/home/clemens/code/notf/res/textures/test.astc", tex_args);

    using VertexLayout   = VertexArray<VertexPos, VertexTexCoord>;
    using InstanceLayout = VertexArray<InstanceXform>;
    using Library        = PrefabGroup<VertexLayout, InstanceLayout>;
    Library library(blinn_phong_shader);

    using Factory = PrefabFactory<Library>;
    Factory factory(library);
    std::shared_ptr<PrefabType<Factory::InstanceData>> box_type;
    {
        auto box = Factory::Box{};
        factory.add(box);
        box_type = factory.produce("boxy_the_box");
    }

    auto box1    = box_type->create_instance();
    box1->data() = std::make_tuple(Xform3f::translation(-500, 500, -1000).as_array());

    auto box2    = box_type->create_instance();
    box2->data() = std::make_tuple(Xform3f::translation(500, 500, -1000).as_array());

    auto box3    = box_type->create_instance();
    box3->data() = std::make_tuple(Xform3f::translation(-500, -500, -1000).as_array());

    auto box4    = box_type->create_instance();
    box4->data() = std::make_tuple(Xform3f::translation(500, -500, -1000).as_array());

    //    auto some_stick = stick_type->create_instance();

    library.init();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    blinn_phong_shader->set_uniform("s_texture", 0);

    // render loop
    using namespace std::chrono_literals;
    auto last_frame_start_time = std::chrono::high_resolution_clock::now();
    float angle                = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        angle += 0.01 * ((frame_start_time - last_frame_start_time) / 16ms);
        last_frame_start_time = frame_start_time;

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        Size2i buffer_size;
        glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
        glViewport(0, 0, buffer_size.width, buffer_size.height);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            auto texture_scope = texture->scope();
            UNUSED(texture_scope);

            // pass the shader uniforms
            const Xform3f move = Xform3f::translation(0, 0, -500);
            //        const Xform3f rotate    = Xform3f::rotation(Vector4f(sin(angle), cos(angle)), angle);
            const Xform3f rotate = Xform3f::rotation(Vector4f(0, 1), angle);
            //        const Xform3f rotate    = Xform3f::identity();
            const Xform3f scale     = Xform3f::scaling(200);
            const Xform3f modelview = move * rotate * scale;
            blinn_phong_shader->set_uniform("modelview", modelview);

            const Xform3f perspective = Xform3f::perspective(deg_to_rad(90), 1, 0, 10000.f);
            //         const Xform3f perspective = Xform3f::orthographic(-1000.f, 1000.f, -1000.f, 1000.f, 0.f, 10000.f);
            blinn_phong_shader->set_uniform("projection", perspective);

            Xform3f normalMat = rotate;
            //        blinn_phong_shader->set_uniform("normalMat", normalMat);

            library.render();

            check_gl_error();
        }

        /////////////////

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, texWidth, texHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderTexture);

        library.render();

        /////////////////

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto sleep_time = max(0ms, 16ms - (std::chrono::high_resolution_clock::now() - frame_start_time));
        std::this_thread::sleep_for(sleep_time);
    }

    // clean up
    glDeleteRenderbuffers(1, &depthRenderbuffer);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &renderTexture);

    context->clear_shader();
}

int main(int /*argc*/, char* /*argv*/ [])
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
