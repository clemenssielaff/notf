#include <iostream>

#include "app/widget.hpp"
#include "app/application.hpp"
using namespace signal;

int main()
{
    Application& app = Application::instance();
    std::shared_ptr<Widget> outer = app.create_widget();
    Handle blub;
    {
        std::shared_ptr<Widget> inner = app.create_widget();
//        inner->set_parent(outer);
        blub = inner->get_handle();
    }

    std::shared_ptr<Widget> inner = app.get_widget(blub);
    inner = app.get_widget(blub);

    return 0;
}
