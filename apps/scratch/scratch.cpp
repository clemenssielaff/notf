#include <iostream>
#include <string>
#include <vector>

#include "notf/meta/macros.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

struct Factory;
struct MasterHandle;
struct Node;
struct Wrapper;

struct Node {

    Node(Factory* p_factory, std::string p_name) : factory(p_factory), name(std::move(p_name))
    {
        std::cout << "created node " << name << std::endl;
    }

    ~Node() { std::cout << "deleted node " << name << std::endl; }

    Factory* factory;
    std::string name;
};

struct Factory {
    Wrapper create();
    size_t counter = 1;
    std::vector<std::shared_ptr<Node>> nodes;
};

struct MasterHandle {

    ~MasterHandle()
    {
        assert(node);
        node->factory->nodes.erase(std::find(node->factory->nodes.begin(), node->factory->nodes.end(), node));
    }

    std::shared_ptr<Node> node;
};

struct Wrapper {
    NOTF_NO_COPY_OR_ASSIGN(Wrapper);
    Wrapper(std::shared_ptr<Node>&& node) : node(std::move(node)) {}
    operator MasterHandle() { return MasterHandle{std::move(node)}; }

private:
    std::shared_ptr<Node> node;
};

Wrapper Factory::create()
{
    auto result = std::make_shared<Node>(this, std::to_string(counter++));
    nodes.emplace_back(result);
    return Wrapper{std::move(result)};
}

struct Base {

protected:
    template<class T, class Forbidden = std::tuple<>>
    Wrapper _create_child_of()
    {
        return _create_child();
    }

private:
public:
    Wrapper _create_child();
};

struct Sub : public Base {
protected:
    Wrapper _create_child() { return Factory().create(); }
};

struct FSub : public Sub {
    Wrapper _create_child() { return Sub::_create_child(); }
};

struct A {};
struct B : public A {};
struct C : public A {};

int main()
{
    Factory factory;

    { // anonymous
        factory.create();
    }

    { // master handle
        MasterHandle handle = factory.create();
    }

    { // nope
        auto nope = factory.create();
    }

    auto sub = FSub();
    sub._create_child();

    auto b = B();
    std::vector<C> vec;
//    vec.emplace_back(std::move(b));

    return 0;
}
