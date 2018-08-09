#include "smoke_example.hpp"

#include <iostream>
#include <thread>

#include "app/application.hpp"
#include "app/node_property.hpp"
#include "app/root_node.hpp"
#include "app/scene.hpp"
#include "common/timer_pool.hpp"
#include "app/widget/painter.hpp"
#include "app/widget/widget_scene.hpp"
#include "app/widget/widget_visualizer.hpp"
#include "app/window.hpp"
#include "auxiliary/visualizer/procedural.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/text/font.hpp"
#include "utils/literals.hpp"

NOTF_USING_NAMESPACE;

#pragma clang diagnostic ignored "-Wweak-vtables"

// == Cloud Scene =================================================================================================== //

struct CloudScene : public Scene {
    CloudScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name)), p_time(_get_root_access().create_property<float>("time", 0))
    {
        m_timer = IntervalTimer::create([&] {
            const auto age_in_ms
                = std::chrono::duration_cast<std::chrono::milliseconds>(TheApplication::get_age()).count();
            p_time.set(static_cast<float>(age_in_ms / 1000.));
        });
        using namespace notf::literals;
        m_timer->start(30_fps); // TODO: 60_fps doesn't work...?
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
    WindowWidget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent) : Widget(token, scene, parent)
    {
        m_font = Font::load(TheGraphicsSystem::get().get_font_manager(), "Roboto-Regular.ttf", 11);
    }

    /// Destructor.
    ~WindowWidget() override = default;

protected:
    void _paint_line(Painter& painter) const
    {
        const Size2i window_size = get_scene().get_window()->get_buffer_size();
        const Vector2f center = Vector2f(window_size.width, window_size.height) / 2;
        const double time = [&]() -> double {
            const auto age_in_ms
                = std::chrono::duration_cast<std::chrono::milliseconds>(TheApplication::get_age()).count();
            return static_cast<double>(age_in_ms);
        }();

        const double length = 200;
        const double period = 30;

        const double half_length = length / 2;
        const double t = fmod(time / (1000 * period), 1) * pi<double>();
        const double sin_t = sin(t);
        const double cos_t = cos(t);
        const Vector2f half_line{sin_t * half_length, cos_t * half_length};
        const CubicBezier2f spline({CubicBezier2f::Segment::line(center + half_line, center - half_line)});

        // draw the rotating line
        painter.set_stroke_width(1);
        painter.set_path(spline);
        painter.stroke();
    }

    void _paint_text(Painter& /*painter*/) const
    {
        // painter.set_font(m_font);
        // painter.write(std::to_string(time / 1000));
    }

    void _paint_shape(Painter& painter) const
    {
        // convex
        //        painter.set_path(
        //            Polygonf({Vector2f{10, 70}, Vector2f{5, 20}, Vector2f{5, 5}, Vector2f{75, 5}, Vector2f{75, 75}}));
        // concave
        painter.set_path(Polygonf({
            Vector2f{565 * 0.6 + 50., 770 * 0.6},
            Vector2f{040 * 0.6 + 50., 440 * 0.6},
            Vector2f{330 * 0.6 + 50., 310 * 0.6},
            Vector2f{150 * 0.6 + 50., 120 * 0.6},
            Vector2f{460 * 0.6 + 50., 230 * 0.6},
            Vector2f{770 * 0.6 + 50., 120 * 0.6},
            Vector2f{250 * 0.6 + 50., 450 * 0.6},
        }));
        painter.fill();
    }

    /// Updates the Design of this Widget through the given Painter.
    void _paint(Painter& painter) const override { _paint_shape(painter); }

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    void _get_widgets_at(const Vector2f&, std::vector<valid_ptr<Widget*>>&) const final {}

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    FontPtr m_font;
};

// == Scene O' Widgets ============================================================================================== //

struct SceneOWidgets : public WidgetScene {
    SceneOWidgets(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : WidgetScene(token, graph, std::move(name)), p_time(_get_root_access().create_property<float>("time", 0))
    {
        set_widget<WindowWidget>();

        m_timer = IntervalTimer::create([&] {
            const auto age_in_ms
                = std::chrono::duration_cast<std::chrono::milliseconds>(TheApplication::get_age()).count();
            p_time.set(static_cast<float>(age_in_ms / 1000.));
        });
        using namespace notf::literals;
        m_timer->start(30_fps);
    }

    ~SceneOWidgets() override { m_timer->stop(); }

private: // fields
    PropertyHandle<float> p_time;
    IntervalTimerPtr m_timer;
};

// == Main ========================================================================================================== //

int smoke_main(int argc, char* argv[])
{
    // initialize the application
    TheApplication::Args args;
    args.argc = argc;
    args.argv = argv;
#ifdef NOTF_MSVC
    args.shader_directory = "C:/Users/Clemens/Code/notf/res/shaders";
    args.texture_directory = "C:/Users/Clemens/Code/notf/res/textures";
#endif // NOTF_MSVC
    TheApplication& app = TheApplication::initialize(args);

    { // initialize the window
        Window::Settings window_settings;
        window_settings.is_resizeable = true;
        WindowPtr window = TheApplication::get().create_window(window_settings);

        //        std::shared_ptr<CloudScene> cloud_scene = Scene::create<CloudScene>(window->get_scene_graph(),
        //        "clouds_scene");
        std::shared_ptr<SceneOWidgets> widget_scene
            = Scene::create<SceneOWidgets>(window->get_scene_graph(), "SceneO'Widgets");

        SceneGraph::CompositionPtr composition = SceneGraph::Composition::create({

            // TODO: this seems exessively complicated to create a Widget Layer ... is it?
            SceneGraph::Layer::create(widget_scene, std::make_unique<WidgetVisualizer>(window->get_graphics_context())),
            //            SceneGraph::Layer::create(cloud_scene, std::make_unique<ProceduralVisualizer>(*window,
            //            "clouds.frag")),
        });
        window->get_scene_graph()->change_composition(composition);
    }
    return app.exec();
}
