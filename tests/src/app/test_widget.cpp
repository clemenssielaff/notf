#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/widget/widget_compiletime.hpp"

NOTF_USING_NAMESPACE;

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
    using super_t = CompileTimeWidget<TestCompileTimeWidgetPolicy>;
    NOTF_UNUSED TestCompileTimeWidget(valid_ptr<Node*> parent) : super_t(parent) {}
};

#ifdef NOTF_MSVC
constexpr auto state_a_id = make_string_type<StateA::name>();
#else
constexpr auto state_a_id = "state_a"_id;
#endif

} // namespace

SCENARIO("Compile Time Widgets", "[app][node][widget]") {
    // always reset the graph
    TheGraph::AccessFor<Tester>::reset();
    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    NodeHandle root_node_handle = TheGraph()->get_root_node();
    RootNodePtr root_node_ptr = std::static_pointer_cast<RootNode>(to_shared_ptr(root_node_handle));
    REQUIRE(root_node_ptr);
    auto root_node = Node::AccessFor<Tester>(*root_node_ptr);

    SECTION("basic state machine") {
        auto widget = root_node.create_child<TestCompileTimeWidget>().to_handle();
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_a"); // first state is the default

        to_shared_ptr(widget)->transition_into<StateB>();
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_b");

        to_shared_ptr(widget)->transition_into("state_c");
        REQUIRE(to_shared_ptr(widget)->get_state_name() == "state_c");

        to_shared_ptr(widget)->transition_into(state_a_id);

        // A -> C is not allowed
        REQUIRE_THROWS_AS(to_shared_ptr(widget)->transition_into<StateC>(), AnyWidget::BadTransitionError);
    }
}
