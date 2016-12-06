#include "core/foo.hpp"

#include "common/log.hpp"

namespace notf {

Foo::~Foo()
{
    log_trace << "Removing Foo instance";
}

void Foo::do_foo() const
{
    log_warning << "Oh no! I'm a Foo";
}

void Bar::do_foo() const
{
    log_info << "Bar foo";
}

std::vector<std::shared_ptr<Foo>> FooCollector::foos = {};

void add_foo(std::shared_ptr<Foo> foo)
{
    FooCollector::foos.push_back(foo);
}

void do_the_foos()
{
    FooCollector().do_the_foos();
}

}
