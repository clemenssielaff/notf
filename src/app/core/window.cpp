#include "app/core/window.hpp"

#include "app/core/application.hpp"
#include "app/core/glfw.hpp"
#include "app/core/resource_manager.hpp"
#include "app/scene/scene_manager.hpp"
#include "app/scene/widget/window_layout.hpp"
#include "common/log.hpp"
#include "graphics/core/raw_image.hpp"
#include "utils/make_smart_enabler.hpp"

namespace { // anonymous
// using namespace notf;
/*
/// Helper function that propagates an event up the Item hierarchy.
/// @param widget    Widget receiving the original event.
/// @param signal    Signal to trigger on each ScreenItem in the hierarchy.
/// @param event     Event object that is passed as an argument to the Signals.
/// @param notified  ScreenItems that have already been notified of this event and that must not handle it again.
/// @return          The ScreenItem that handled the event or nullptr, if none did.
///
template <class Event, typename... Args>
ScreenItem* propagate_to_hierarchy(Widget* widget, Signal<Args...> ScreenItem::*signal, Event& event,
                                   std::unordered_set<ScreenItem*>& notified_items)
{
    ScreenItem* screen_item = widget;
    while (screen_item) {
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
        screen_item = screen_item->get_layout();
    }
    return nullptr;
}
*/
} // namespace

namespace notf {

namespace detail {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

} // namespace detail

//====================================================================================================================//

window_initialization_error::~window_initialization_error() {}

//====================================================================================================================//

std::shared_ptr<Window> Window::create(const Args& args)
{
    WindowPtr window;
#ifdef NOTF_DEBUG
    window = WindowPtr(new Window(args));
#else
    window = std::make_shared<make_shared_enabler<Window>>(args);
#endif
    log_info << "Created Window '" << window->title() << "' "
             << "using OpenGl version: " << glGetString(GL_VERSION);

    // inititalize the window
    Application::Access<Window>().register_new(window);
    return window;
}

Window::Window(const Args& args)
    : m_glfw_window(nullptr, detail::window_deleter)
    , m_title(args.title)
    , m_layout()
    , m_scene_manager() // has to be created after the OpenGL Context
    , m_size(args.size)
{
    // make sure that an Application was initialized before instanciating a Window (will throw on failure)
    Application& app = Application::instance();

    // NoTF uses OpenGL ES 3.2
    //    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, args.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(m_size.width, m_size.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        notf_throw_format(window_initialization_error,
                          "Window or OpenGL context creation failed for Window '" << title() << "'");
    }
    glfwSetWindowUserPointer(m_glfw_window.get(), this);

    // create the auxiliary objects
    m_scene_manager = SceneManager::create(m_glfw_window.get());

    // apply the Window icon
    // In order to show the icon in Ubuntu 16.04 is as bit more complicated:
    // 1. start the software at least once, you might have to pin it to the launcher as well ...
    // 2. copy the icon to ~/.local/share/icons/hicolor/<resolution>/apps/<yourapp>
    // 3. modify the file ~/.local/share/applications/<yourapp>, in particular the line Icon=<yourapp>
    //
    // in order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!args.icon.empty()) {
        const std::string icon_path = app.resource_manager().texture_directory() + args.icon;
        try {
            RawImage icon(icon_path);
            if (icon.channels() != 4) {
                log_warning << "Icon file '" << icon_path << "' does not provide the required 4 byte per pixel, but "
                            << icon.channels();
            }
            else {
                const GLFWimage glfw_icon{icon.width(), icon.height(), const_cast<uchar*>(icon.data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
            }
        }
        catch (std::runtime_error) {
            log_warning << "Failed to load Window icon '" << icon_path << "'";
        }
    }

    // create the layout
    m_layout = WindowLayout::Access<Window>::create(this);
    WindowLayout::Access<Window>(*m_layout).set_grant(buffer_size());
}

Window::~Window() { close(); }

Size2i Window::framed_window_size() const
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

Size2i Window::buffer_size() const
{
    if (!m_glfw_window) {
        return Size2i::invalid();
    }
    Size2i result;
    glfwGetFramebufferSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

Vector2f Window::mouse_pos() const
{
    if (!m_glfw_window) {
        return Vector2f::zero();
    }
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);
    return {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};
}

void Window::request_redraw() const
{
    // TODO: Window::request_redraw
}

void Window::close()
{
    if (m_glfw_window) {
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        Application::Access<Window>().unregister(this);
        m_layout.reset();
        m_scene_manager.reset();
        m_glfw_window.reset();
    }
    m_size = Size2i::invalid();
}

void Window::_resize(Size2i size)
{
    m_size = std::move(size);
    //    m_layout->_set_grant(get_buffer_size());
}

void Window::_propagate(MouseEvent&& event)
{
    //    // sort Widgets by RenderLayers
    //    std::vector<std::vector<Widget*>> widgets_by_layer(m_render_manager->get_layer_count());
    //    for (Widget* widget : m_layout->get_widgets_at(event.window_pos)) {
    //        RenderLayer* render_layer = widget->get_render_layer().get();
    //        assert(render_layer);
    //        widgets_by_layer[render_layer->get_index()].emplace_back(widget);
    //    }

    //    WidgetPtr mouse_item = m_mouse_item.lock();
    //    std::unordered_set<ScreenItem*> notified_items;

    //    // call the appropriate event signal
    //    if (event.action == MouseAction::MOVE) {
    //        if (mouse_item) {
    //            mouse_item->on_mouse_move(event);
    //            if (event.was_handled()) {
    //                return;
    //            }
    //            notified_items.insert(mouse_item.get());
    //        }
    //        for (const auto& layer : reverse(widgets_by_layer)) {
    //            for (const auto& widget : layer) {
    //                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_move, event, notified_items)) {
    //                    return;
    //                }
    //            }
    //        }
    //    }

    //    else if (event.action == MouseAction::SCROLL) {
    //        if (mouse_item) {
    //            mouse_item->on_mouse_scroll(event);
    //            if (event.was_handled()) {
    //                return;
    //            }
    //            notified_items.insert(mouse_item.get());
    //        }
    //        for (const auto& layer : reverse(widgets_by_layer)) {
    //            for (const auto& widget : layer) {
    //                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_scroll, event, notified_items)) {
    //                    return;
    //                }
    //            }
    //        }
    //    }

    //    else if (event.action == MouseAction::PRESS) {
    //        assert(!mouse_item);
    //        for (const auto& layer : reverse(widgets_by_layer)) {
    //            for (const auto& widget : layer) {
    //                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_button, event, notified_items)) {
    //                    WidgetPtr new_focus_widget = make_shared_from(widget);
    //                    m_mouse_item               = new_focus_widget;

    //                    // do nothing if the item already has the focus
    //                    WidgetPtr old_focus_widget = m_keyboard_item.lock();
    //                    if (new_focus_widget == old_focus_widget) {
    //                        return;
    //                    }

    //                    // send the mouse item a 'focus gained' event and notify its hierarchy, if it handles it
    //                    FocusEvent focus_gained_event(*this, FocusAction::GAINED, old_focus_widget, new_focus_widget);
    //                    new_focus_widget->on_focus_changed(focus_gained_event);
    //                    if (focus_gained_event.was_handled()) {

    //                        // let the previously focused Widget know that it lost the focus
    //                        if (old_focus_widget) {
    //                            FocusEvent focus_lost_event(*this, FocusAction::LOST, old_focus_widget,
    //                            new_focus_widget); ScreenItem* handler = old_focus_widget.get(); while (handler) {
    //                                handler->on_focus_changed(focus_lost_event);
    //                                handler = handler->get_layout();
    //                            }
    //                        }

    //                        // notify the new focused Widget's hierarchy
    //                        m_keyboard_item     = new_focus_widget;
    //                        ScreenItem* handler = new_focus_widget->get_layout();
    //                        while (handler) {
    //                            handler->on_focus_changed(focus_gained_event);
    //                            handler = handler->get_layout();
    //                        }
    //                    }
    //                    else {
    //                        // if it doesn't handle the focus event, the focus remains untouched
    //                    }

    //                    return;
    //                }
    //            }
    //        }
    //    }

    //    else if (event.action == MouseAction::RELEASE) {
    //        if (mouse_item) {
    //            m_mouse_item.reset();
    //            mouse_item->on_mouse_button(event);
    //            if (event.was_handled()) {
    //                return;
    //                =
    //            }
    //            notified_items.insert(mouse_item.get());
    //        }
    //        for (const auto& layer : reverse(widgets_by_layer)) {
    //            for (const auto& widget : layer) {
    //                if (propagate_to_hierarchy(widget, &ScreenItem::on_mouse_button, event, notified_items)) {
    //                    return;
    //                }
    //            }
    //        }
    //    }

    //    else {
    //        assert(0);
    //    }
}

void Window::_propagate(KeyEvent&& event)
{
    //    std::unordered_set<ScreenItem*> notified_items;
    //    if (WidgetPtr keyboard_item = m_keyboard_item.lock()) {
    //        propagate_to_hierarchy(keyboard_item.get(), &ScreenItem::on_key, event, notified_items);
    //    }
    //    else { // if there is no keyboard item, only notify the WindowLayout
    //        m_layout->on_key(event);
    //    }
}

void Window::_propagate(CharEvent&& event)
{
    //    std::unordered_set<ScreenItem*> notified_items;
    //    if (WidgetPtr keyboard_item = m_keyboard_item.lock()) {
    //        propagate_to_hierarchy(keyboard_item.get(), &ScreenItem::on_char_input, event, notified_items);
    //    }
    //    else { // if there is no keyboard item, only notify the WindowLayout
    //        m_layout->on_char_input(event);
    //    }
}

void Window::_update()
{
    assert(m_glfw_window);

    // do nothing, if there are no Widgets in need to be redrawn
    //    if (m_render_manager->is_clean()) { // TODO: dirty mechanism for SceneManager
    //        return;
    //    }

    // make the window current
    Application::Access<Window>().set_current(this);

    // render
    try {
        m_scene_manager->render();
    }
    // if an error bubbled all the way up here, something has gone horribly wrong
    catch (const notf_exception& error) {
        log_critical << "Rendering failed: \"" << error.what() << "\"";
    }
}

} // namespace notf
