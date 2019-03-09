#include <iostream>

#include "notf/meta/log.hpp"
#include "notf/meta/time.hpp"

#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/path2.hpp"
#include "notf/common/geo/polybezier.hpp"
#include "notf/common/thread.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/graphic/plotter/painter.hpp"

#include "notf/app/application.hpp"
#include "notf/app/driver.hpp"
#include "notf/app/event_handler.hpp"
#include "notf/app/graph/slot.hpp"
#include "notf/app/graph/window.hpp"
#include "notf/app/timer_pool.hpp"

#include "notf/app/widget/widget.hpp"
#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// super widget ===================================================================================================== //

class SuperWidget;

namespace super_widget {

struct SuperProp {
    using value_t = float;
    static constexpr ConstString name = "super_prop";
    static inline const value_t default_value = 1;
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REFRESH;
};

struct SingleState : State<SingleState, SuperWidget> {
    static constexpr ConstString name = "single_state";
    explicit SingleState(SuperWidget& node) : SingleState::super_t(node) {}
};

struct Policy {
    using properties = std::tuple<SuperProp>;
    using slots = std::tuple<>;
    using signals = std::tuple<>;
    using states = std::variant<SingleState>;
};

} // namespace super_widget

class SuperWidget : public Widget<super_widget::Policy> {

private:
    using super_t = Widget<super_widget::Policy>;

public:
    static constexpr const ConstString& super_prop = super_widget::SuperProp::name;

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
                    const float time
                        = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(get_age()).count());
                    const float period = 10;
                    const float t = fmod(time / (1000.f * period), 1.f);
                    const float angle = t * pi<float>() * 2.f;
                    if (handle.is_valid()) {
                        auto xform = M3f::translation({200, 200}) * M3f::rotation(angle);
                        handle->set<super_prop>(t);
                        handle->set<offset_xform>(xform);
                    }
                });
            }
        });
        m_animation->start();
    }

    void _paint(Painter& painter) const override {
        //        const float half_length = 100.f; // * get<super_prop>();
        //        const V2f half_line{half_length, half_length};
        //        const Path2Ptr spline = ;

        // draw a rectangle
        painter.set_stroke_width(5.f);
        //        painter.set_paint(Color::red());
        painter.set_path(Path2::rect(Aabrf(20, 20, 50, 50)));
        //        painter.stroke();

        // draw a complex shape
        painter.set_joint_style(Painter::JointStyle::ROUND);
        painter.set_path(Path2::create(Polylinef{V2f{120, 60}, V2f{160, 400},  //
                                                 V2f{200, 120}, V2f{240, 280}, //
                                                 V2f{280, 160}, V2f{340, 200}, //
                                                 V2f{380, 180}, V2f{420, 190}, //
                                                 V2f{500, 380}, V2f{350, 400}, //
                                                 V2f{380, 320}}));
        //        painter.set_path(Path2::rect(Aabrf(120, 120, 50, 50)));
        painter.stroke();

        // draw a background
        //        painter.set_path(convert_to<Polylinef>(Aabrf(-half_length, -half_length, half_length * 2, half_length
        //        * 2))); painter.stroke();

        //        const CubicBezier2f spline2({CubicBezier2f::Segment::line(-half_line - V2f{100.f, 0}, half_line)});

        //        painter.set_path(Path2::rect());
        //        painter *= M3f::translation(400, 0);
        //        painter.stroke();
    }
    void _relayout() override {}
    void _get_widgets_at(const V2f&, std::vector<WidgetHandle>&) const override {}

private:
    TimerPtr m_animation;
};

// main ============================================================================================================= //

int run_main(int argc, char* argv[]) {
    // initialize application
    TheApplication::Arguments arguments("Scratch1", argc, argv);
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
