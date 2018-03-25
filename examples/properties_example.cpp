#include "renderthread_example.hpp"

#include "app/application.hpp"
#include "app/scene_node.hpp"
#include "app/property_graph.hpp"
#include "app/window.hpp"
#include "common/log.hpp"

NOTF_USING_NAMESPACE

class A {
public:
    virtual int get() const { return 3; }
};

class B : public A {};

class C : public A {};

class Antity : public SceneNode {
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE
    Antity(const Token& token, ChildContainerPtr container) : SceneNode(token, std::move(container)) {}

public:
    static std::shared_ptr<Antity> create()
    {
        return SceneNode::_create<Antity>(std::make_unique<detail::EmptyNodeContainer>());
    }
    virtual void _remove_child(const SceneNode*) override {}
};

int properties_main(int argc, char* argv[])
{
    Application::initialize(argc, argv);
    auto& app = Application::instance();

    Window::Args window_args;
    window_args.icon = "notf.png";
    auto window = Window::create(window_args);

    auto prop_a = Property<int>(3);
    auto prop_b = Property<int>(3);

    auto b = B{};
    auto c = C{};
    std::vector<std::reference_wrapper<A>> vec;
    vec.push_back(b);
    vec.push_back(c);

    for (A& a : vec) {
        log_trace << a.get();
    }

    auto ant = Antity::create();

    return app.exec();
}
