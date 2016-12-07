#pragma once

#include <memory>
#include <vector>

struct _object;
using PyObject = _object;

namespace notf {

void decref_pyobject(PyObject* object);

class Foo : public std::enable_shared_from_this<Foo> {
public:
    Foo();
    virtual ~Foo();
    virtual void do_foo() const = 0;

    void set_pyobject(PyObject* object);

    std::unique_ptr<PyObject, decltype(&decref_pyobject)> py_object; // TODO: this crashes obviously if Python objects are still alife after the interpreter shut down
};

class Bar : public Foo {
public:
    using Foo::Foo;
    virtual void do_foo() const override;
};

struct FooCollector {
    static std::vector<std::shared_ptr<Foo>> foos;

    void clear_the_foos()
    {
        foos.clear();
    }

    void do_the_foos()
    {
        for (const auto& foo : foos) {
            foo->do_foo();
        }
    }
};

void add_foo(std::shared_ptr<Foo> foo);

void do_the_foos();

} // namespace notf
