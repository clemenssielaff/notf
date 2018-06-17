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
    CloudScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name)), p_time(_root().create_property<float>("time", 0))
    {
        m_timer = IntervalTimer::create([&] {
            const auto age_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Application::age()).count();
            p_time.set(static_cast<float>(age_in_ms / 1000.));
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
    Application::Args args;
    args.argc = argc;
    args.argv = argv;
#ifdef NOTF_MSVC
    args.shader_directory = "C:/Users/Clemens/Code/notf/res/shaders";
    args.texture_directory = "C:/Users/Clemens/Code/notf/res/textures";
#endif // NOTF_MSVC
    Application& app = Application::initialize(args);

    { // initialize the window
        WindowPtr window = Application::instance().create_window();
        auto scene = Scene::create<CloudScene>(window->scene_graph(), "clouds_scene");

        auto renderer = ProceduralRenderer::create(*window, "clouds.frag");
        std::vector<valid_ptr<LayerPtr>> layers = {Layer::create(*window, std::move(renderer), scene)};
        SceneGraph::StatePtr state = window->scene_graph()->create_state(std::move(layers));
        window->scene_graph()->enter_state(state);
    }
    return app.exec();
}
