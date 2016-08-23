#if 1

#include "common/signal.hpp"
#include "core/application.hpp"
#include "core/shadercomponent.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "core/key_event.hpp"

using namespace signal;

int main(void)
{
    WindowInfo window_info;
    window_info.enable_vsync = false; // to correctly time each frame
    Window window(window_info);

    {
//        std::shared_ptr<Widget> widget = Widget::make_widget();
//        widget->set_parent(window.get_root_widget());

//        std::shared_ptr<ShaderComponent> shader = make_component<ShaderComponent>();
//        widget->set_component(shader);
    }

    window.on_token_key.connect([&](const KeyEvent&){

        for(auto i = 0; i < 1; ++i){
            std::shared_ptr<Widget> widget = Widget::make_widget();
            widget->set_parent(window.get_root_widget());

            std::shared_ptr<ShaderComponent> shader = make_component<ShaderComponent>();
            widget->set_component(shader);
        }

    }, [](const KeyEvent& event){
        return event.action != KEY_ACTION::KEY_RELEASE && event.key == KEY::SPACE;
    });

    return Application::get_instance().exec();
}

#endif
