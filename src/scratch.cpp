#if 1

#include <string>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/window.hpp"
#include "graphics/graphics_manager.hpp"
using namespace signal;

int main()
{
    Window window;
    GraphicsManager gm("/home/clemens/temp/", "/home/clemens/temp/");
    {
        auto bla = gm.get_texture("awesomeface2.png");
        {
            gm.get_texture("awesomeface.png");
        }
        gm.cleanup();
    }
    gm.cleanup();

    return 0;
}
#endif
