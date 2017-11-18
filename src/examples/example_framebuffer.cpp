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
    using type                     = Vector4f;
    using kind                     = AttributeKind::Position;
};

struct VertexNormal : public AttributeTrait {
    constexpr static uint location = 1;
    using type                     = Vector4f;
    using kind                     = AttributeKind::Normal;
};

struct VertexTexCoord : public AttributeTrait {
    constexpr static uint location = 2;
    using type                     = Vector2f;
    using kind                     = AttributeKind::TexCoord;
};

struct InstanceXform : public AttributeTrait {
    constexpr static uint location = 3;
    using type                     = Matrix4f;
    using kind                     = AttributeKind::Other;
};
} // namespace

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

    /////////////////////////////////////

    FrameBufferPtr framebuffer;
    TexturePtr render_target;
    {
        Texture::Args targs;
        targs.min_filter = Texture::MinFilter::LINEAR;
        render_target    = Texture::create_empty(graphics_context, "render_target", {800, 800}, targs);

        FrameBuffer::Args fargs;
        fargs.color_targets = {std::make_pair(0, FrameBuffer::ColorTarget{render_target})};

        framebuffer = std::make_shared<FrameBuffer>(*graphics_context, std::move(fargs));
    }

    /////////////////////////////////////

    const std::string vertex_src     = load_file("/home/clemens/code/notf/res/shaders/blinn_phong.vert");
    VertexShaderPtr blinn_phong_vert = VertexShader::build(graphics_context, "Blinn-Phong.vert", vertex_src.c_str());

    const std::string frag_src         = load_file("/home/clemens/code/notf/res/shaders/blinn_phong.frag");
    FragmentShaderPtr blinn_phong_frag = FragmentShader::build(graphics_context, "Blinn-Phong.frag", frag_src.c_str());

    PipelinePtr blinn_phong_pipeline = Pipeline::create(graphics_context, blinn_phong_vert, blinn_phong_frag);
    graphics_context->bind_pipeline(blinn_phong_pipeline);

    Texture::Args tex_args;
    tex_args.codec      = Texture::Codec::ASTC;
    tex_args.anisotropy = 5;
    TexturePtr texture  = Texture::load_image(graphics_context, "/home/clemens/code/notf/res/textures/test.astc",
                                             "testtexture", tex_args);

    using VertexLayout   = VertexArray<VertexPos, VertexTexCoord>;
    using InstanceLayout = VertexArray<InstanceXform>;
    using Library        = PrefabGroup<VertexLayout, InstanceLayout>;
    Library library;

    using Factory = PrefabFactory<Library>;
    Factory factory(library);
    std::shared_ptr<PrefabType<Factory::InstanceData>> box_type;
    {
        auto box = Factory::Box{};
        factory.add(box);
        box_type = factory.produce("boxy_the_box");
    }

    auto box1    = box_type->create_instance();
    box1->data() = std::make_tuple(Matrix4f::translation(-500, 500, -1000));

    auto box2    = box_type->create_instance();
    box2->data() = std::make_tuple(Matrix4f::translation(500, 500, -1000));

    auto box3    = box_type->create_instance();
    box3->data() = std::make_tuple(Matrix4f::translation(-500, -500, -1000));

    auto box4    = box_type->create_instance();
    box4->data() = std::make_tuple(Matrix4f::translation(500, -500, -1000));

    library.init();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // render loop
    using namespace std::chrono_literals;
    auto last_frame_start_time = std::chrono::high_resolution_clock::now();
    float angle                = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        angle += 0.01 * ((frame_start_time - last_frame_start_time) / 16ms);
        last_frame_start_time = frame_start_time;

        graphics_context->bind_framebuffer(framebuffer);

        Size2i buffer_size;
        glfwGetFramebufferSize(window, &buffer_size.width, &buffer_size.height);
        glViewport(0, 0, buffer_size.width, buffer_size.height);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            graphics_context->bind_texture(texture, 0);

            // pass the shader uniforms
            const Matrix4f move = Matrix4f::translation(0, 0, -500);
            //        const Xform3f rotate    = Xform3f::rotation(Vector4f(sin(angle), cos(angle)), angle);
            const Matrix4f rotate = Matrix4f::rotation(Vector3f(0, 1), angle);
            //        const Xform3f rotate    = Xform3f::identity();
            const Matrix4f scale     = Matrix4f::scaling(200);
            const Matrix4f modelview = move * rotate * scale;
            blinn_phong_vert->set_uniform("modelview", modelview);

            const Matrix4f perspective = Matrix4f::perspective(deg_to_rad(90), 1, 0, 10000.f);
            //         const Xform3f perspective = Xform3f::orthographic(-1000.f, 1000.f, -1000.f, 1000.f, 0.f,
            //         10000.f);
            blinn_phong_vert->set_uniform("projection", perspective);

            //            graphics_context->bind_pipeline(blinn_phong_pipeline);
            library.render();

            graphics_context->unbind_texture(0);
            gl_check_error();
        }

        /////////////////

        graphics_context->unbind_framebuffer();

        glViewport(0, 0, 800, 800);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        graphics_context->bind_texture(render_target, 0);
        //        glGenerateMipmap(GL_TEXTURE_2D);

        library.render();

        /////////////////

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto sleep_time = max(0ms, 16ms - (std::chrono::high_resolution_clock::now() - frame_start_time));
        std::this_thread::sleep_for(sleep_time);
    }

    // clean up
    graphics_context->unbind_all_textures();
    graphics_context->unbind_framebuffer();
    graphics_context->unbind_pipeline();
}

int framebuffer_main(int /*argc*/, char* /*argv*/ [])
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
