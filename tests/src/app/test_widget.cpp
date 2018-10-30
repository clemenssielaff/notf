#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/widget/widget_compiletime.hpp"

NOTF_USING_NAMESPACE;
NOTF_USING_LITERALS;

namespace {

struct StateA;
struct StateB;
struct StateC;
struct TestCompileTimeWidget;

struct StateA : State<StateA, TestCompileTimeWidget> {
    static constexpr StringConst name = "state_a";
    NOTF_UNUSED explicit StateA(TestCompileTimeWidget& node) : StateA::base_t(node) {}
    NOTF_UNUSED explicit StateA(StateC&& c) : StateA::base_t(this->_get_node(c)) {}
};

struct StateB : State<StateB, TestCompileTimeWidget> {
    static constexpr StringConst name = "state_b";
    NOTF_UNUSED explicit StateB(StateA&& a) : StateB::base_t(this->_get_node(a)) {}
};

struct StateC : State<StateC, TestCompileTimeWidget> {
    static constexpr StringConst name = "state_c";
    NOTF_UNUSED explicit StateC(StateB&& b) : StateC::base_t(this->_get_node(b)) {}
};

struct TestCompileTimeWidgetPolicy {
    using properties = std::tuple< //
        FloatPropertyPolicy,       //
        IntPropertyPolicy,         //
        BoolPropertyPolicy>;       //

    using states = std::variant< //
        StateA,                  //
        StateB,                  //
        StateC>;                 //
};

struct TestCompileTimeWidget : public CompileTimeWidget<TestCompileTimeWidgetPolicy> {
    NOTF_UNUSED TestCompileTimeWidget(valid_ptr<Node*> parent) : CompileTimeWidget<TestCompileTimeWidgetPolicy>(parent)
    {}
};

} // namespace

SCENARIO("Compile Time Widgets", "[app][node][widget]")
{
    // always reset the graph
    TheGraph::initialize<TestRootNode>();
    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    NodeHandle root_node = TheGraph::get_root_node();
    auto root_node_ptr = std::dynamic_pointer_cast<TestRootNode>(to_shared_ptr(root_node));
    REQUIRE(root_node);

    SECTION("basic state machine")
    {
        auto widget = root_node_ptr->create_child<TestCompileTimeWidget>().to_handle();
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_a"); // first state is the default

        to_shared_ptr(widget)->transition_into<StateB>();
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_b");

        to_shared_ptr(widget)->transition_into("state_c");
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_c");

        to_shared_ptr(widget)->transition_into("state_a"_id);

        // A -> C is not allowed
        REQUIRE_THROWS_AS(to_shared_ptr(widget)->transition_into<StateC>(), AnyWidget::BadTransitionError);
    }
}
