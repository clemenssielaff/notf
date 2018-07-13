#include "smoke_example.hpp"

#include <iostream>
#include <thread>

#include "app/application.hpp"
#include "app/node_property.hpp"
#include "app/root_node.hpp"
#include "app/scene.hpp"
#include "app/timer_manager.hpp"
#include "app/widget/painter.hpp"
#include "app/widget/widget_scene.hpp"
#include "app/widget/widget_visualizer.hpp"
#include "app/window.hpp"
#include "auxiliary/visualizer/procedural.hpp"

NOTF_USING_NAMESPACE;

#pragma clang diagnostic ignored "-Wweak-vtables"

// == Cloud Scene =================================================================================================== //

struct CloudScene : public Scene {
    CloudScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name)), p_time(_get_root_access().create_property<float>("time", 0))
    {
        m_timer = IntervalTimer::create([&] {
            const auto age_in_ms
                = std::chrono::duration_cast<std::chrono::milliseconds>(Application::get_age()).count();
            p_time.set(static_cast<float>(age_in_ms / 1000.));
        });

        using namespace notf::literals;
        m_timer->start(20_fps);
    }

    ~CloudScene() override { m_timer->stop(); }

    void _resize_view(Size2i) override {}

private: // fields
    PropertyHandle<float> p_time;
    IntervalTimerPtr m_timer;
};

// == Window Widget ================================================================================================= //

class WindowWidget : public Widget {
    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    WindowWidget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent) : Widget(token, scene, parent) {}

    /// Destructor.
    ~WindowWidget() override = default;

protected:
    /// Updates the Design of this Widget through the given Painter.
    void _paint(Painter& painter) const override
    {
        painter.translate(50, 50);
        painter.line_to(100, 100);
        painter.stroke();
    }

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    void _get_widgets_at(const Vector2f&, std::vector<valid_ptr<Widget*>>&) const final {}
};

// == Scene O' Widgets ============================================================================================== //

struct SceneOWidgets : public WidgetScene {
    SceneOWidgets(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : WidgetScene(token, graph, std::move(name))
    {
        set_widget<WindowWidget>();
    }
};

// == Main ========================================================================================================== //

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

        std::shared_ptr<CloudScene> cloud_scene = Scene::create<CloudScene>(window->get_scene_graph(), "clouds_scene");
        std::shared_ptr<SceneOWidgets> widget_scene
            = Scene::create<SceneOWidgets>(window->get_scene_graph(), "SceneO'Widgets");

        SceneGraph::CompositionPtr composition = SceneGraph::Composition::create({
            SceneGraph::Layer::create(widget_scene, std::make_unique<WidgetVisualizer>(*window)),
            SceneGraph::Layer::create(cloud_scene, std::make_unique<ProceduralVisualizer>(*window, "clouds.frag")),
        });
        window->get_scene_graph()->change_composition(composition);
    }
    return app.exec();
}
