#include "core/window.hpp"

#include <unordered_set>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/events/char_event.hpp"
#include "core/events/focus_event.hpp"
#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/glfw.hpp"
#include "core/render_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "core/window_layout.hpp"
#include "graphics/cell/cell_canvas.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/raw_image.hpp"
#include "utils/reverse_iterator.hpp"

namespace { // anonymous
using namespace notf;

/** Helper function that propagates an event up the Item hierarchy.
 * @param widget    Widget receiving the original event.
 * @param signal    Signal to trigger on each ScreenItem in the hierarchy.
 * @param event     Event object that is passed as an argument to the Signals.
 * @param notified  ScreenItems that have already been notified of this event and that must not handle it again.
 * @return          The ScreenItem that handled the event or nullptr, if none did.
 */
template <class Event, typename... Args>
ScreenItem* propagate_to_hierarchy(Widget* widget, Signal<Args...> ScreenItem::*signal, Event& event,
                                   std::unordered_set<ScreenItem*>& notified_items)
{
    ScreenItemPtr handler = make_shared_from(widget);
    while (handler) {
        ScreenItem* screen_item = handler.get();

        // don't propagate the event to items that have already seen (but not handled) it
        if (notified_items.count(screen_item)) {
            return nullptr;
        }
        notified_items.insert(screen_item);

        // fire the signal and return if it was handled
        (screen_item->*signal)(event);
        if (event.was_handled()) {
            return screen_item;
        }
        handler = handler->get_layout();
    }
    return nullptr;
}

} // namespace anonymous

namespace notf {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    struct make_shared_enabler : public Window {
        make_shared_enabler(const WindowInfo& info)
            : Window(info) {}
    };
    std::shared_ptr<Window> window = std::make_shared<make_shared_enabler>(info);

    if (get_gl_error()) {
        exit(to_number(Application::RETURN_CODE::OPENGL_FAILURE));
    }
    else {
        log_info << "Created Window '" << window->get_title() << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }

    // inititalize the window
    Application::get_instance()._register_window(window);
    window->m_layout = WindowLayout::create(window);
    window->m_layout->_set_size(window->get_buffer_size());

    return window;
}

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr, window_deleter)
    , m_title(info.title)
    , m_layout()
    , m_render_manager() // has to be created after the OpenGL Context
    , m_graphics_context()
    , m_cell_context()
    , m_background_color(info.clear_color)
    , m_size(info.size)
{
    // make sure that an Application was initialized before instanciating a Window (will exit on failure)
    Application& app = Application::get_instance();

    // NoTF uses OpenGL ES 3.0 for now
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(m_size.width, m_size.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        log_fatal << "Window or OpenGL context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::GLFW_FAILURE));
    }
    glfwSetWindowUserPointer(m_glfw_window.get(), this);
    glfwMakeContextCurrent(m_glfw_window.get());
    glfwSwapInterval(app.get_info().enable_vsync ? 1 : 0);

    // create the auxiliary objects
    m_render_manager = std::make_unique<RenderManager>(this);

    GraphicsContextOptions context_args;
    context_args.pixel_ratio = static_cast<float>(get_buffer_size().width) / static_cast<float>(get_window_size().width);
    m_graphics_context       = std::make_unique<GraphicsContext>(this, context_args);

    m_cell_context = std::make_unique<CellCanvas>(*m_graphics_context);

    // apply the Window icon
    // In order to show the icon in Ubuntu 16.04 is as bit more complicated:
    // 1. start the software at least once, you might have to pin it to the launcher as well ...
    // 2. copy the icon to ~/.local/share/icons/hicolor/<resolution>/apps/<yourapp>
    // 3. modify the file ~/.local/share/applications/<yourapp>, in particular the line Icon=<yourapp>
    //
    // in order to remove leftover icons on Ubuntu call:
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
        } catch (std::runtime_error) {
            log_warning << "Failed to load Window icon '" << icon_path << "'";
        }
    }
}

Window::~Window()
{
    close();
}

Size2i Window::get_framed_window_size() const
{
    if (!m_glfw_window) {
        return Size2i::invalid();
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
        return Size2i::invalid();
    }
    Size2i result;
    glfwGetFramebufferSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

Vector2f Window::get_mouse_pos() const
{
    if (!m_glfw_window) {
        return Vector2f::zero();
    }
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);
    return {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};
}

void Window::_update()
{
    assert(m_glfw_window);

    // do nothing, if there are no Widgets in need to be redrawn
    //    if (m_render_manager->is_clean()) { // TODO: dirty mechanism for RenderManager
    //        return;
    //    }

    // make the window current
    Application::get_instance()._set_current_window(this);

    // prepare the viewport
    Size2i buffer_size;
    glfwGetFramebufferSize(m_glfw_window.get(), &buffer_size.width, &buffer_size.height);

    glViewport(0, 0, buffer_size.width, buffer_size.height);
    glClearColor(m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // render
    try {
        m_render_manager->render(buffer_size);
    }
    // if an error bubbled all the way up here, something has gone horribly wrong
    catch (std::runtime_error error) {
        log_critical << "Rendering failed: \"" << error.what() << "\"";
    }

    // post-render
    glfwSwapBuffers(m_glfw_window.get());
}

void Window::close()
{
    if (m_glfw_window) {
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_layout.reset();
        Application::get_instance()._unregister_window(shared_from_this());
        m_glfw_window.reset(nullptr);
    }
    m_size = Size2i::invalid();
}

void Window::_on_resize(int width, int height)
{
    m_size.width  = width;
    m_size.height = height;
    m_layout->_set_size(get_buffer_size());
}

void Window::_propagate_mouse_event(MouseEvent&& event)
{
    // sort Widgets by RenderLayers
    std::vector<std::vector<Widget*>> widgets_by_layer(m_render_manager->get_layer_count());
    for (Widget* widget : m_layout->get_widgets_at(event.window_pos)) {
        RenderLayer* render_layer = widget->get_render_layer().get();
        assert(render_layer);
        widgets_by_layer[render_layer->get_index()].emplace_back(widget);
    }

    std::shared_ptr<Widget> mouse_item = m_mouse_item.lock();
    std::unordered_set<ScreenItem*> notified_items;

    // call the appropriate event signal
    if (event.action == MouseAction::MOVE) {
        if (mouse_item) {
            mouse_item->on_mouse_move(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& layer : reverse(widgets_by_layer)) {
            for (const auto& widget : layer) {
                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_move, event, notified_items)) {
                    return;
                }
            }
        }
    }

    else if (event.action == MouseAction::SCROLL) {
        if (mouse_item) {
            mouse_item->on_mouse_scroll(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& layer : reverse(widgets_by_layer)) {
            for (const auto& widget : layer) {
                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_scroll, event, notified_items)) {
                    return;
                }
            }
        }
    }

    else if (event.action == MouseAction::PRESS) {
        assert(!mouse_item);
        for (const auto& layer : reverse(widgets_by_layer)) {
            for (const auto& widget : layer) {
                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_button, event, notified_items)) {
                    WidgetPtr new_focus_widget = make_shared_from(widget);
                    m_mouse_item               = new_focus_widget;

                    // send the mouse item a 'focus gained' event and notify its hierarchy, if it handles it
                    WidgetPtr old_focus_widget = m_keyboard_item.lock();
                    FocusEvent focus_gained_event(*this, FocusAction::GAINED, old_focus_widget, new_focus_widget);
                    new_focus_widget->on_focus_changed(focus_gained_event);
                    if (focus_gained_event.was_handled()) {

                        // let the previously focused Widget know that it lost the focus
                        if (old_focus_widget) {
                            FocusEvent focus_lost_event(*this, FocusAction::LOST, old_focus_widget, new_focus_widget);
                            ScreenItemPtr handler = old_focus_widget;
                            while (handler) {
                                handler->on_focus_changed(focus_lost_event);
                                handler = handler->get_layout();
                            }
                        }

                        // notify the new focused Widget's hierarchy
                        m_keyboard_item       = new_focus_widget;
                        ScreenItemPtr handler = new_focus_widget->get_layout();
                        while (handler) {
                            handler->on_focus_changed(focus_gained_event);
                            handler = handler->get_layout();
                        }
                    }
                    else {
                        // if it doesn't handle the focus event, the focus remains untouched
                    }

                    return;
                }
            }
        }
    }

    else if (event.action == MouseAction::RELEASE) {
        if (mouse_item) {
            m_mouse_item.reset();
            mouse_item->on_mouse_button(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& layer : reverse(widgets_by_layer)) {
            for (const auto& widget : layer) {
                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_button, event, notified_items)) {
                    return;
                }
            }
        }
    }

    else {
        assert(0);
    }
}

void Window::_propagate_key_event(KeyEvent&& event)
{
}

void Window::_propagate_char_event(CharEvent&& event)
{
}

} // namespace notf
