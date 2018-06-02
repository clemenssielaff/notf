#include "smoke_example.hpp"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "app/application.hpp"
#include "app/layer.hpp"
#include "app/render/procedural.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"

NOTF_USING_NAMESPACE

struct CloudScene : public Scene {
    CloudScene(const FactoryToken& token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name))
    {}
    void _resize_view(Size2i) override {}
};

int smoke_main(int argc, char* argv[])
{
    // initialize the application
    Application& app = Application::initialize(argc, argv);

    // initialize the window
    Window& window = Application::instance().create_window();
    {
        auto scene = Scene::create<CloudScene>(window.scene_graph(), "clouds_scene");

        auto renderer = ProceduralRenderer::create(window, "clouds.frag");
        std::vector<valid_ptr<LayerPtr>> layers = {Layer::create(window, std::move(renderer), scene)};
        SceneGraph::StatePtr state = window.scene_graph()->create_state(std::move(layers));
        window.scene_graph()->enter_state(state);
    }
    return app.exec();
}
