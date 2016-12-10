#include <iostream>
using namespace std;

#include "core/controller.hpp"
#include "core/widget.hpp"
#include "utils/make_smart_enabler.hpp"
using namespace notf;

class BoomWidget : public Widget {
public:
    virtual void paint(Painter&) const override {}
protected:
    BoomWidget() : Widget() {}
};

class Dynamite : public Controller<Dynamite> {
public:
    Dynamite()
        : Controller<Dynamite>(
              std::make_shared<MakeSmartEnabler<BoomWidget>>(),
              init_state_machine())
    {
        transition_to(m_state_calm);
        cout << "Starting in State: " << get_current_state_name() << endl;
    }

    void go_boom() { transition_to(m_state_boom); }
private:
    const std::string& get_explosive() const
    {
        static const std::string explosive = "dynamite";
        return explosive;
    }

    StateMachine init_state_machine()
    {
        StateMachine state_machine;

        m_state_calm = state_machine.add_state(
            "calm",
            [](Dynamite& self) { cout << "I'm loaded with " << self.get_explosive() << endl; }, // enter
            [](Dynamite&) { cout << "Tick tick tick..." << endl; }); // leave

        m_state_boom = state_machine.add_state(
            "kaboom",
            [](Dynamite&) { cout << "Kaboom" << endl; }, // enter
            {}); // leave

        return state_machine;
    }

private:
    const State* m_state_calm;
    const State* m_state_boom;
};

//int main()
int notmain()
{
    Dynamite stick;
    stick.go_boom();
    return 0;
}
