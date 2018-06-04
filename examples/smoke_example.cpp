#include "smoke_example.hpp"

#include <iostream>
#include <thread>

#include "app/application.hpp"
#include "app/layer.hpp"
#include "app/node_property.hpp"
#include "app/render/procedural.hpp"
#include "app/root_node.hpp"
#include "app/scene.hpp"
#include "app/timer_manager.hpp"
#include "app/window.hpp"

NOTF_USING_NAMESPACE

#pragma clang diagnostic ignored "-Wweak-vtables"

struct CloudScene : public Scene {
    CloudScene(const FactoryToken& token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name)), p_time(_root().create_property<float>("time", 0))
    {
        m_timer = IntervalTimer::create([&] {
            const float last_time = p_time.value();
            p_time.set_value(last_time + 1.f / 20.0f); // TODO: get global time
        });

        using namespace notf::literals;
        m_timer->start(20_fps);
    }
    void _resize_view(Size2i) override {}

private: // fields
    PropertyHandle<float> p_time;
    IntervalTimerPtr m_timer;
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
