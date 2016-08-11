#if 1
#include "core/application.hpp"
#include "core/window.hpp"
#include "core/widget.hpp"
#include "app/shadercomponent.h"

using namespace signal;

int main(void)
{
    Window window;

    std::shared_ptr<Widget> widget = Widget::make_widget();
    widget->set_parent(window.get_root_widget());

    std::shared_ptr<ShaderComponent> shader = make_component<ShaderComponent>();
    widget->set_component(shader);

    return Application::get_instance().exec();
}

#endif
