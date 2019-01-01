#include <iostream>

#include "notf/meta/log.hpp"
#include "notf/meta/time.hpp"

#include "notf/common/thread.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/app/application.hpp"
#include "notf/app/driver.hpp"
#include "notf/app/event_handler.hpp"
#include "notf/app/graph/slot.hpp"
#include "notf/app/graph/window.hpp"
#include "notf/app/timer_pool.hpp"

#include "notf/app/widget/painter.hpp"
#include "notf/app/widget/widget.hpp"
#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// super widget ===================================================================================================== //

class SuperWidget;

namespace super_widget {

struct Rotation {
    using value_t = double;
    static constexpr ConstString name = "rotation";
    static inline const value_t default_value = 0.2;
    static constexpr bool is_visible = true;
};

struct SingleState : State<SingleState, SuperWidget> {
    static constexpr ConstString name = "single_state";
    explicit SingleState(SuperWidget& node) : SingleState::super_t(node) {}
};

struct Policy {
    using properties = std::tuple<Rotation>;
    using slots = std::tuple<>;
    using signals = std::tuple<>;
    using states = std::variant<SingleState>;
};

} // namespace super_widget

class SuperWidget : public Widget<super_widget::Policy> {

private:
    using super_t = Widget<super_widget::Policy>;

public:
    static constexpr const ConstString& Rotation = super_widget::Rotation::name;

public:
    SuperWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {}

    ~SuperWidget() override { m_animation->stop(); }

private:
    void _finalize() override {
        auto raw = handle_from_this();
        NOTF_ASSERT(raw);
        auto handle = handle_cast<NodeHandle<SuperWidget>>(raw);
        m_animation = IntervalTimer(60_fps, [handle]() mutable {
            if (handle.is_valid()) {
                TheEventHandler()->schedule([=]() mutable {
                    const double time
                        = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(get_age()).count());
                    const double period = 10;
                    if (handle.is_valid()) { handle->set<Rotation>(fmod(time / (1000 * period), 1) * pi<double>()); }
                });
            }
        });
        m_animation->start();
    }

    void _paint(Painter& painter) const override {

        //        const Size2i window_size = get_scene().get_window()->get_buffer_size();
        const Size2i window_size = {400, 400};
        const V2f center = V2f{window_size.width(), window_size.height()} / 2;

        const double length = 200;

        const double half_length = length / 2;
        const double t = get<Rotation>();
        const double sin_t = sin(t);
        const double cos_t = cos(t);
        const V2f half_line{sin_t * half_length, cos_t * half_length};
        const CubicBezier2f spline({CubicBezier2f::Segment::line(center + half_line, center - half_line)});
        // const CubicBezier2f spline({CubicBezier2f::Segment::line(Vector2f(20, 20), Vector2f(327, 174))});

        // draw the rotating line
        painter.set_stroke_width(1.2f);
        painter.set_path(spline);
        painter.stroke();
    }
    void _relayout() override {}
    void _get_widgets_at(const V2f&, std::vector<WidgetHandle>&) const override {}

private:
    TimerPtr m_animation;
};

// main ============================================================================================================= //

int run_main(int argc, char* argv[]) {
    // initialize application
    TheApplication::Arguments arguments;
    arguments.argc = argc;
    arguments.argv = argv;
    TheApplication app(std::move(arguments));

    WindowHandle window = Window::create();
    WidgetSceneHandle scene = window->set_scene<WidgetScene>();
    scene->set_widget<SuperWidget>();

    NOTF_ASSERT(window->get_scene());

    const int result = app->exec();
    return result;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
