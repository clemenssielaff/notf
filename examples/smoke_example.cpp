#include "smoke_example.hpp"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "app/application.hpp"
#include "app/layer.hpp"
#include "app/render/procedural.hpp"
#include "app/scene_manager.hpp"
#include "app/window.hpp"

NOTF_USING_NAMESPACE

int smoke_main(int argc, char* argv[])
{
    // initialize the application
    Application& app = Application::initialize(argc, argv);

    // initialize the window
    Window& window = Application::instance().create_window();
    {
        auto renderer = ProceduralRenderer::create(window, "clouds.frag");
        std::vector<LayerPtr> layers = {Layer::create(window, std::move(renderer))};
        SceneManager::StatePtr state = window.scene_manager().create_state(layers);
        window.scene_manager().enter_state(state);
    }
    return app.exec();

}
