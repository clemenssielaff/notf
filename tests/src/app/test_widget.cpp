#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/widget/widget_compiletime.hpp"

NOTF_USING_NAMESPACE;

namespace {

template<class>
struct StateA;
template<class>
struct StateB;
template<class>
struct StateC;

enum StateIdentifier {
    INVALID = 0,
    A,
    B,
    C,
};

template<class NodeType>
struct StateA : State<StateA, NodeType> {
    explicit StateA(NodeType& node) : StateA::base_t(node) {}
    explicit StateA(StateC<NodeType>&& c) : StateA::base_t(this->_get_node(c)) {}
    StateIdentifier callback() const { return A; }
};

template<class NodeType>
struct StateB : State<StateB, NodeType> {
    explicit StateB(StateA<NodeType>&& a) : StateB::base_t(this->_get_node(a)) {}
    StateIdentifier callback() const { return B; }
};

template<class NodeType>
struct StateC : State<StateC, NodeType> {
    explicit StateC(StateB<NodeType>&& b) : StateC::base_t(this->_get_node(b)) {}
    StateIdentifier callback() const { return C; }
};

struct TestCompileTimeWidgetPolicy {
    using node_t = CompileTimeWidget<TestCompileTimeWidgetPolicy>;

    using properties = std::tuple< //
        FloatPropertyPolicy,       //
        IntPropertyPolicy,         //
        BoolPropertyPolicy>;       //

    using states = std::variant< //
        StateA<node_t>,          //
        StateB<node_t>,          //
        StateC<node_t>>;         //
};

struct TestCompileTimeWidget : public CompileTimeWidget<TestCompileTimeWidgetPolicy> {

    NOTF_UNUSED TestCompileTimeWidget(valid_ptr<Node*> parent) : CompileTimeWidget<TestCompileTimeWidgetPolicy>(parent)
    {}

    NOTF_UNUSED StateIdentifier get_state() const
    {
        StateIdentifier id = INVALID;
        std::visit(overloaded{[&id](auto&& state) { id = state.callback(); }}, _get_state());
        return id;
    }
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
        REQUIRE(to_shared_ptr(widget)->get_state() == A); // first state is the default

        to_shared_ptr(widget)->transition_into<StateB<CompileTimeWidget<TestCompileTimeWidgetPolicy>>>();
        REQUIRE(to_shared_ptr(widget)->get_state() == B);

        to_shared_ptr(widget)->transition_into(2);
        REQUIRE(to_shared_ptr(widget)->get_state() == C);

        to_shared_ptr(widget)->transition_into(0);
        REQUIRE(to_shared_ptr(widget)->get_state() == A);

        // A -> C is not allowed
        REQUIRE_THROWS_AS(to_shared_ptr(widget)->transition_into(2), AnyWidget::BadTransitionError);
    }
}
