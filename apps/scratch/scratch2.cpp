#include <iostream>

#include "notf/app/application.hpp"
#include "notf/app/widget/widget.hpp"
#include "notf/meta/log.hpp"
NOTF_USING_NAMESPACE;

class PooWidget;

namespace poolicy {

struct StateA;
struct StateB;
struct StateC;

struct Int {
    using value_t = int;
    static constexpr ConstString name = "Int";
    static inline const value_t default_value = 123;
    static constexpr bool is_visible = true;
};

struct Float {
    using value_t = float;
    static constexpr ConstString name = "Float";
    static inline const value_t default_value = {};
    static constexpr bool is_visible = true;
};

struct SuperSlot {
    using value_t = None;
    static constexpr ConstString name = "to_super";
};

struct SuperSignal {
    using value_t = int;
    static constexpr ConstString name = "on_super";
};

struct StateA : State<StateA, PooWidget> {
    static constexpr ConstString name = "state_a";
    NOTF_UNUSED explicit StateA(PooWidget& node) : StateA::super_t(node) {}
    NOTF_UNUSED explicit StateA(StateC&& c) : StateA::super_t(this->_get_node(c)) {}
};

struct StateB : State<StateB, PooWidget> {
    static constexpr ConstString name = "state_b";
    NOTF_UNUSED explicit StateB(StateA&& a) : StateB::super_t(this->_get_node(a)) {}
};

struct StateC : State<StateC, PooWidget> {
    static constexpr ConstString name = "state_c";
    NOTF_UNUSED explicit StateC(StateB&& b) : StateC::super_t(this->_get_node(b)) {}
};

struct SuperPolicy {
    using properties = std::tuple< //
        Int,                       //
        Float                      //
        >;                         //

    using slots = std::tuple< //
        SuperSlot             //
        >;                    //

    using signals = std::tuple< //
        SuperSignal             //
        >;                      //

    using states = std::variant< //
        StateA,                  //
        StateB,                  //
        StateC                   //
        >;                       //
};

} // namespace poolicy

class PooWidget : public Widget<poolicy::SuperPolicy> {
    using super_t = Widget<poolicy::SuperPolicy>;

public:
    static constexpr const ConstString& Int = poolicy::Int::name;
    static constexpr const ConstString& Float = poolicy::Float::name;

public:
    PooWidget() : super_t(this) {}

    void _paint(Painter&) const override {}
    void _relayout() override {}
    void _get_widgets_at(const V2f&, std::vector<WidgetHandle>&) const override {}
};

int main(int argc, char* argv[]) {
    // disable console output of the logger
    TheLogger::Arguments args;
    args.console_level = TheLogger::Level::OFF;
    TheLogger::initialize(args);

    // initialize application
    TheApplication::Arguments arguments;
    arguments.argc = argc;
    arguments.argv = argv;
    TheApplication app(std::move(arguments));

    PooWidget poo;
    std::cout << poo.get<PooWidget::Int>() << std::endl;

    poo.set<PooWidget::Int>(42);
    std::cout << poo.get<PooWidget::Int>() << std::endl;

    std::cout << "-----" << std::endl;

    std::cout << poo.get<AnyWidget::opacity>() << std::endl;

    poo.set<AnyWidget::opacity>(0.42f);
    std::cout << poo.get<AnyWidget::opacity>() << std::endl;

    return 0;
}
