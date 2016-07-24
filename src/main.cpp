#include "app/application.hpp"
#include "app/window.hpp"
#include "common/signal.hpp"

using namespace signal;

int main(void)
{
    Application& app = Application::instance();

    Window window;
//    if (app.has_errors()) {
//        return -1;
//    }

    // Create a window and its OpenGL context
//    app.create_window();
//    if (app.has_errors()) {
//        return -1;
//    }

    return app.exec();
}
