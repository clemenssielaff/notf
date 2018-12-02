#include <iostream>
#include <memory>

struct Node {
    void public_a() { std::cout << "public A" << std::endl; }
    void public_b() { std::cout << "public B" << std::endl; }
    void private_c() { std::cout << "private C" << std::endl; }
};
using NodePtr = std::shared_ptr<Node>;

struct SuperNode : public Node {
    void public_super_a() { std::cout << "public super A" << std::endl; }
};
using SuperPtr = std::shared_ptr<SuperNode>;

template<class T>
struct CommonInterface : protected T {
    using T::public_a;
    using T::public_b;
};
template<class T>
struct Interface : public CommonInterface<T> {};
using NodeInterface = Interface<Node>;

template<>
struct Interface<SuperNode> : public CommonInterface<SuperNode> {
    using SuperNode::public_super_a;
};
using SuperInterface = Interface<SuperNode>;

int main() {
    NodePtr nodeptr = std::make_shared<SuperNode>();
    auto iptr = std::static_pointer_cast<NodeInterface>(nodeptr);
    auto sptr = std::static_pointer_cast<SuperInterface>(nodeptr);

    iptr->public_a();
    iptr->public_b();
    //    iptr->private_c();

    sptr->public_a();
    sptr->public_b();
    sptr->public_super_a();

    return 0;
}
