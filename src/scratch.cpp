#include <string>

#define SIGNAL_LOG_LEVEL SIGNAL_LOG_LEVEL_CRITICAL

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
using namespace signal;

//int main()
int log_test()
{
    Window window;
    std::shared_ptr<Widget> widget = Widget::make_widget();
    widget->set_parent(window.get_root_widget());
    for(size_t i=0; i < 10000000; ++i){
        log_warning << "Derbe" << widget->get_handle();
    }

    return 0;
}
