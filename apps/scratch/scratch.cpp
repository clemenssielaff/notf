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

#include "notf/app/widget/layout.hpp"
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

struct SingleState : State<SingleState, ParentWidget> {
    static constexpr ConstString name = "single_state";
    explicit SingleState(ParentWidget& node) : SingleState::super_t(node) {}
};

struct Policy {
    using properties = std::tuple<FloatProperty>;
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

private:
    void _finalize() override {}

    void _get_widgets_at(const V2f&, std::vector<WidgetHandle>&) const override {}

    void _paint(Painter& painter) const override {
        // draw a rectangle
        painter.set_stroke_width(2.f);
        painter.set_joint_style(Painter::JointStyle::BEVEL);
        painter.set_path(Path2::rect(get_grant()));
        painter.stroke();
    }
};

// parent widget ==================================================================================================== //

class ParentWidget : public Widget<test_widget::Policy> {

private:
    using super_t = Widget<test_widget::Policy>;

public:
    static constexpr const ConstString& float_property = test_widget::FloatProperty::name;

public:
    ParentWidget(valid_ptr<AnyNode*> parent) : super_t(parent) {
        OverLayout& layout = _set_layout<OverLayout>();
        layout.set_padding(Paddingf::all(100));

        _create_child<ChildWidget>(this);
    }

private:
    void _finalize() override {}

    void _get_widgets_at(const V2f&, std::vector<WidgetHandle>&) const override {}

    void _paint(Painter& painter) const override {}
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
