#pragma once

#include <memory>
#include <vector>

namespace notf {

class Foo : public std::enable_shared_from_this<Foo> {
public:
    Foo() = default;
    virtual ~Foo();
    virtual void do_foo() const = 0;
};

class Bar : public Foo {
public:
    using Foo::Foo;
    virtual void do_foo() const override;
};

struct FooCollector{
    static std::vector<std::shared_ptr<Foo>> foos;

    void do_the_foos()
    {
        for(const auto& foo : foos){
            foo->do_foo();
        }
    }
};


void add_foo(std::shared_ptr<Foo> foo);

void do_the_foos();

} // namespace notf
