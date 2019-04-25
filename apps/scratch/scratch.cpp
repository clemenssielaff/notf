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
#include "notf/app/event_handler.hpp"
#include "notf/app/graph/slot.hpp"
#include "notf/app/graph/window.hpp"
#include "notf/app/graph/window_driver.hpp"
#include "notf/app/timer_pool.hpp"

#include "notf/app/widget/widget.hpp"
#include "notf/app/widget/widget_scene.hpp"

NOTF_OPEN_NAMESPACE

// super widget ===================================================================================================== //

class ParentWidget;

namespace test_widget {

struct FloatProperty {
    using value_t = float;
    static constexpr ConstString name = "float_property";
    static inline const value_t default_value = 1;
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REFRESH;
};

struct SizeProperty {
    using value_t = Size2f;
    static constexpr ConstString name = "size_property";
    static inline const value_t default_value = Size2f(50, 50);
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REFRESH;
};

struct SingleState : State<SingleState, ParentWidget> {
    static constexpr ConstString name = "single_state";
    explicit SingleState(ParentWidget& node) : SingleState::super_t(node) {}
};

struct Policy {
    using properties = std::tuple<FloatProperty, SizeProperty>;
    using slots = std::tuple<>;
    using signals = std::tuple<>;
    using states = std::variant<SingleState>;
};

} // namespace test_widget

// child widget ===================================================================================================== //

class ChildWidget : public Widget<test_widget::Policy> {

private:
    using super_t = Widget<test_widget::Policy>;

public:
    ChildWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {}
};

// parent widget ==================================================================================================== //

class ParentWidget : public Widget<test_widget::Policy> {

private:
    using super_t = Widget<test_widget::Policy>;

public:
    static constexpr const ConstString& float_property = test_widget::FloatProperty::name;
    static constexpr const ConstString& size_property = test_widget::SizeProperty::name;

public:
    ParentWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {}

    ~ParentWidget() override {
        if (m_animation) { m_animation->stop(); }
    }

private:
    void _finalize() override {

        set<size_property>(Size2f(280, 200));
        set<offset_xform>(M3f::translation(200, 200));

//        auto raw = handle_from_this();
//        NOTF_ASSERT(raw);
//        auto handle = handle_cast<NodeHandle<ParentWidget>>(raw);
//        m_animation = IntervalTimer(60_fps, [handle]() mutable {
//            if (handle.is_valid()) {
//                TheEventHandler()->schedule([=]() mutable {
//                    const float time
//                        = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(get_age()).count());
//                    const float period = 10;
//                    const float t = fmod(time / (1000.f * period), 1.f);
//                    //                    const float angle = t * pi<float>() * 2.f;
//                    if (handle.is_valid()) {
//                        //                        auto xform = M3f::translation({200, 200}) * M3f::rotation(angle);
////                        handle->set<float_property>(t);
////                        handle->eset<offset_xform>(M3f::translation(20, 20));
//                        // TODO: why is this required in order for the widget to be drawn at all?
//                        //                        handle->set<offset_xform>(xform);
//                    }
//                });
//            }
//        });
//        m_animation->start();
    }

    void _paint(Painter& painter) const override {
        // draw a rectangle
        painter.set_stroke_width(5.f);
        painter.set_joint_style(Painter::JointStyle::BEVEL);
        painter.set_path(Path2::rect(Aabrf(get<size_property>())));
        painter.stroke();

        // draw a complex shape
        //        painter.set_stroke_width(20.f);
        //        painter.set_cap_style(Painter::CapStyle::ROUND);
        //        painter.set_joint_style(Painter::JointStyle::ROUND);
        //        painter.set_path(Path2::create(Polylinef{V2f{120, 60}, V2f{160, 400},  //
        //                                                 V2f{200, 120}, V2f{240, 280}, //
        //                                                 V2f{280, 160}, V2f{340, 200}, //
        //                                                 V2f{380, 180}, V2f{420, 190}, //
        //                                                 V2f{500, 380}, V2f{350, 400}, //
        //                                                 V2f{380, 320}}));
        //        painter.stroke();
    }
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
    scene->set_widget<ParentWidget>();

    NOTF_ASSERT(window->get_scene());

    const int result = app->exec();
    return result;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
