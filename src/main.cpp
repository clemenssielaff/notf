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
    Window window;



    window.on_token_key.connect([&](const KeyEvent& event){

        for(auto i = 0; i < 100; ++i){
            std::shared_ptr<Widget> widget = Widget::make_widget();
            widget->set_parent(window.get_root_widget());

            std::shared_ptr<ShaderComponent> shader = make_component<ShaderComponent>();
            widget->set_component(shader);
        }

    }, [](const KeyEvent& event){
        return event.action == KEY_ACTION::KEY_PRESS;
    });

    return Application::get_instance().exec();
}

#endif
