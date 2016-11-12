#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"

#include "common/const.hpp"
#include "common/log.hpp"
#include "core/components/canvas_component.hpp"
#include "core/layout_root.hpp"
#include "core/widget.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "python/interpreter.hpp"
#include "graphics/painter.hpp"

using namespace notf;


class TestPainter : public Painter {
protected:
    virtual void paint() override {
        start_path();
        set_fill(Color(1, 1, 1.f));
        circle(50, 50, 50);
        fill();
    }
};


void blub(std::shared_ptr<Window> window)
{
    // components
    std::shared_ptr<CanvasComponent> canvas_component = make_component<CanvasComponent>();
    auto painter = std::make_shared<TestPainter>();
    canvas_component->set_painter(painter);

    // background (blue)
    std::shared_ptr<Widget> background_widget = Widget::create();
    background_widget->add_component(canvas_component);

    window->get_layout_root()->set_item(background_widget);

    log_info << "HIT IT!";
    return;

//    // left widget (green)
//    std::shared_ptr<Widget> left_widget = Widget::create();
//    left_widget->add_component(canvas_component);

//    // right widget (red)
//    std::shared_ptr<Widget> right_widget = Widget::create();
//    right_widget->add_component(canvas_component);

//    // horizontal layout
//    std::shared_ptr<StackLayout> horizontal_layout = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
//    horizontal_layout->add_item(left_widget);
//    horizontal_layout->add_item(right_widget);

//    // vertical layout
//    std::shared_ptr<StackLayout> vertical_layout = StackLayout::create(STACK_DIRECTION::TOP_TO_BOTTOM);
//    window->get_layout_root()->set_item(vertical_layout);
//    vertical_layout->add_item(horizontal_layout);
//    vertical_layout->add_item(background_widget);
}

int main(int argc, char* argv[])
//int notmain(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc = argc;
    app_info.argv = argv;
    app_info.texture_directory = "/home/clemens/code/notf/res/textures";
    app_info.shader_directory = "/home/clemens/code/notf/res/shaders";
    Application& app = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon = "notf.png";
    window_info.width = 800;
    window_info.height = 600;
    window_info.enable_vsync = false;
    std::shared_ptr<Window> window = Window::create(window_info);

    window->on_token_key.connect(
        [window, &app](const KeyEvent&) {
            PythonInterpreter* python = app.get_python_interpreter();
            python->parse_app();
//            blub(window);
        },
        [](const KeyEvent& event) -> bool {
            return event.key == KEY::SPACE && event.action == KEY_ACTION::KEY_PRESS;
        });

    return app.exec();
}
