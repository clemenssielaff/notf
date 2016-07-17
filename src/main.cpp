#include "app/application.hpp"
#include "common/signal.hpp"

using namespace untitled;

int main(void)
{
    Application& app = Application::get_instance();
    if (app.has_errors()) {
        return -1;
    }

    // Create a window and its OpenGL context
    app.create_window();
    if (app.has_errors()) {
        return -1;
    }

    return app.exec();
}
