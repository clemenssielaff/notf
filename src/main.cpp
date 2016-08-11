#if 1
#include "core/application.hpp"
#include "core/window.hpp"

using namespace signal;

int main(void)
{
    Application& app = Application::get_instance();
    Window window;
    return app.exec();
}

#endif
