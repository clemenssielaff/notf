#include "core/window.hpp"

#define NANOVG_GLES3_IMPLEMENTATION
#include "core/glfw_wrapper.hpp"

#include <nanovg/nanovg.h>
#include <nanovg/nanovg_gl.h>

#include <set>

#include "common/debug.hpp"
#include "common/log.hpp"
#include "core/application.hpp"
#include "core/controller.hpp"
#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/raw_image.hpp"
#include "graphics/rendercontext.hpp"
#include "utils/make_smart_enabler.hpp"
#include "utils/unused.hpp"

namespace notf {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

void nanovg_deleter(NVGcontext* nvg_context)
{
    if (nvg_context) {
        nvgDeleteGLES3(nvg_context);
    }
}

Window::~Window()
{
    close();
}

Size2i Window::get_window_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    Size2i result;
    glfwGetWindowSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

Size2i Window::get_framed_window_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    int left, top, right, bottom;
    glfwGetWindowFrameSize(m_glfw_window.get(), &left, &top, &right, &bottom);
    Size2i result = {right - left, bottom - top};
    assert(result.is_valid());
    return result;
}

Size2i Window::get_buffer_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    Size2i result;
    glfwGetFramebufferSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

void Window::_update()
{
    assert(m_glfw_window);

    // do nothing, if there are no Widgets in need to be redrawn
    if (m_render_manager->is_clean()) {
        return;
    }

    // make the window current
    Application::get_instance()._set_current_window(this);

    // build the render context
    RenderContext ctx;
    ctx.nanovg_context = m_nvg_context.get();
    glfwGetWindowSize(m_glfw_window.get(), &ctx.window_size.width, &ctx.window_size.height);
    glfwGetFramebufferSize(m_glfw_window.get(), &ctx.buffer_size.width, &ctx.buffer_size.height);
    ctx.time = Time::now();

    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);
    ctx.mouse_pos = {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};

    // prepare the viewport
    glViewport(0, 0, ctx.buffer_size.width, ctx.buffer_size.height);
    glClearColor(m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // render
    nvgBeginFrame(ctx.nanovg_context, ctx.window_size.width, ctx.window_size.height, ctx.get_pixel_ratio());
    try {
        m_render_manager->render(ctx);
        nvgEndFrame(ctx.nanovg_context);
    }
    // if an error bubbled all the way up here, something has gone horribly wrong
    catch (std::runtime_error error) {
        log_critical << "Rendering failed: \"" << error.what() << "\"";
        nvgCancelFrame(ctx.nanovg_context);
    }

    // post-render
    glEnable(GL_DEPTH_TEST);
    glfwSwapBuffers(m_glfw_window.get());
}

void Window::close()
{
    if (m_glfw_window) {
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_root_layout.reset();
        Application::get_instance()._unregister_window(shared_from_this());
        m_glfw_window.reset(nullptr);
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    std::shared_ptr<Window> window = std::make_shared<MakeSmartEnabler<Window>>(info);
    if (!check_gl_error()) {
        log_info << "Created Window '" << window->get_title() << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }

    // inititalize the window
    Application::get_instance()._register_window(window);
    window->m_root_layout = std::make_shared<MakeSmartEnabler<LayoutRoot>>(window);
    window->m_root_layout->_set_size(Size2f::from_size2i(window->get_buffer_size()));
    // TODO: why again is the LayoutRoot created outside the Window Constructor?

    return window;
}

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr, window_deleter)
    , m_nvg_context(nullptr, nanovg_deleter)
    , m_title(info.title)
    , m_root_layout()
    , m_render_manager(std::make_unique<MakeSmartEnabler<RenderManager>>(this))
    , m_background_color(info.clear_color)
    , m_last_mouse_pos(NAN, NAN)
{
    // always make sure that the Application is constructed first
    Application& app = Application::get_instance();

    // close when the user presses ESC
    connect_signal(on_token_key, &Window::close, [](const KeyEvent& event) { return event.key == Key::ESCAPE; });

    // NoTF uses OpenGL ES 3.0 for now
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        log_fatal << "Window or OpenGL context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::GLFW_FAILURE));
    }
    glfwSetWindowUserPointer(m_glfw_window.get(), this);

    // create the NanoVG context
    glfwMakeContextCurrent(m_glfw_window.get());
    m_nvg_context.reset(nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES
#ifdef _DEBUG
                                       | NVG_DEBUG
#endif
                                       ));
    if (!m_nvg_context) {
        log_fatal << "NanoVG context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::NANOVG_FAILURE));
    }
    app.get_resource_manager().set_nvg_context(m_nvg_context.get());

    // initial OpenGl setup
    glfwSwapInterval(info.enable_vsync ? 1 : 0);

    // apply the Window icon
    // In order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!info.icon.empty()) {
        const std::string icon_path = app.get_resource_manager().get_texture_directory() + info.icon;
        try {
            RawImage icon(icon_path);
            if (icon.get_bytes_per_pixel() != 4) {
                log_warning << "Icon file '" << icon_path
                            << "' does not provide the required 4 byte per pixel, but " << icon.get_bytes_per_pixel();
            }
            else {
                const GLFWimage glfw_icon{icon.get_width(), icon.get_height(), const_cast<uchar*>(icon.get_data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
            }
        }
        catch (std::runtime_error) {
            log_warning << "Failed to load Window icon '" << icon_path << "'";
        }
    }
}

void Window::_on_resize(int width, int height)
{
    UNUSED(width);
    UNUSED(height);
    m_root_layout->_set_size(Size2f::from_size2i(get_buffer_size()));
}

void Window::_on_cursor_move(MouseEvent&& event)
{
    // collect all Widgets in Layout-given order (no RenderLayers yet).
    std::vector<Widget*> widgets;
    m_root_layout->get_widgets_at(event.window_pos, widgets);

    // extract Controllers in order, respecting RenderLayers
    std::set<AbstractController*> known_controllers;
    std::vector<std::vector<AbstractController*>> controllers_by_layer;
    for (Widget* widget : widgets) {

        // notify each Controller only once
        AbstractController* controller = widget->get_controller().get();
        if(known_controllers.count(controller)){
            continue;
        }
        known_controllers.insert(controller);

        // find the widgets's render layer
        RenderLayer* render_layer = widget->get_render_layer().get();
        if(!render_layer){
            const Item* ancestor = widget->get_parent().get();
            assert(ancestor);
            while(!render_layer){
                render_layer = ancestor->get_render_layer().get();
                ancestor = ancestor->get_parent().get();
                assert(ancestor || render_layer);
            }
        }

        // sort them into RenderLayers
        size_t layer_index = m_render_manager->get_render_layer_index(render_layer);
        assert(layer_index > 0);
        if (layer_index > controllers_by_layer.size()) {
            controllers_by_layer.resize(layer_index);
        }
        controllers_by_layer[layer_index - 1].emplace_back(controller);
    }

    // call the event signals
    for (const auto& layer : controllers_by_layer) {
        for (const auto& controller : layer) {
            MouseEvent event(this, {}, Button::INVALID, MouseAction::MOVE, KeyModifiers::NONE, {});
            controller->on_mouse_event(event); //  TODO: send event object to on_mouse_event
        }
    }
}

} // namespace notf
