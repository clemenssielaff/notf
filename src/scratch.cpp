#if 1
#include <iostream>

#include "app/widget.hpp"
#include "app/application.hpp"
using namespace signal;

int main()
{
    Application& app = Application::get_instance();
    std::shared_ptr<Widget> outer = Widget::make_widget();
    std::shared_ptr<Widget> a = Widget::make_widget(1026);
    std::shared_ptr<Widget> b = Widget::make_widget();
    std::shared_ptr<Widget> c = Widget::make_widget();

    Handle blub;
    {
        std::shared_ptr<Widget> inner = Widget::make_widget();
        inner->set_parent(outer);
        blub = inner->get_handle();
    }

    std::shared_ptr<Widget> inner = app.get_widget(blub);
    inner = app.get_widget(blub);

    return 0;
}
#endif
