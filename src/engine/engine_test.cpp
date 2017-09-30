#include "engine/engine_test.hpp"

#include <iostream>

#include "common/half.hpp"
#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/warnings.hpp"
#include "core/glfw.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/prefab_factory.hpp"
#include "graphics/prefab_library.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex_array.hpp"
#include "utils/static_string.hpp"

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

struct InstanceXform {
    StaticString name             = "iOffset";
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

    ShaderPtr blinn_phong_shader = Shader::load(
        *context.get(), "Blinn-Phong",
        "/home/clemens/code/notf/res/shaders/blinn_phong.vert",
        "/home/clemens/code/notf/res/shaders/blinn_phong.frag");
    auto shader_scope = blinn_phong_shader->scope();
    UNUSED(shader_scope);

    using VertexLayout   = VertexArray<VertexPos, VertexNormal>;
    using InstanceLayout = VertexArray<InstanceXform>;
    using Library        = PrefabLibrary<VertexLayout, InstanceLayout>;
    Library library(blinn_phong_shader);

    using Factory = PrefabFactory<Library>;
    Factory factory(library);
    std::shared_ptr<Prefab<Factory::InstanceData>> box_type;
    {
        auto box = Factory::Box{};
        factory.add(box);
        box_type = factory.produce("boxy_the_box");
    }

    std::shared_ptr<Prefab<Factory::InstanceData>> stick_type;
    {
        auto stick   = Factory::Box{};
        stick.width  = 0.5;
        stick.depth  = 0.5;
        stick.height = 4;
        factory.add(stick);
        stick_type = factory.produce("sticky_the_stick");
    }

    auto box1 = box_type->create_instance();
    box1->data() = std::make_tuple(Xform3f::translation(-200, 500, -1000).as_array());

    auto box2 = box_type->create_instance();
    box2->data() = std::make_tuple(Xform3f::translation(200, 500, -1000).as_array());

    auto box3 = box_type->create_instance();
    box3->data() = std::make_tuple(Xform3f::translation(-200, -500, -1000).as_array());

    auto box4 = box_type->create_instance();
    box4->data() = std::make_tuple(Xform3f::translation(200, -500, -1000).as_array());

    //    auto some_stick = stick_type->create_instance();

    library.init();

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
        const Xform3f rotate    = Xform3f::rotation(Vector4f(1, 0), angle);
        const Xform3f scale     = Xform3f::scaling(100);
        const Xform3f modelview = move * rotate * scale;
        blinn_phong_shader->set_uniform("modelview", modelview);

        const Xform3f perspective = Xform3f::perspective(deg_to_rad(90), 1, 0, 10000.f);
        // const Xform3f perspective = Xform3f::orthographic(-400.f, 400.f, -400.f, 400.f, 0.f, 1000.f);
        blinn_phong_shader->set_uniform("projection", perspective);

        Xform3f normalMat = rotate;
        blinn_phong_shader->set_uniform("normalMat", normalMat);

        library.render();

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
