#include <cstdlib>
#include <iostream>

#include "notf/common/dynarray.hpp"

struct Foo {
    ~Foo() {
        if (print) std::cout << message << std::endl;
    }
    std::string message = "Foo deleted";
    static inline bool print = true;
};

static const Foo forever_foo;

int main() {
    {
        auto foos = notf::SharedDynArray<Foo>(5, forever_foo);
        std::cout << "Hello, notf!" << std::endl;
        foos[2].message = "something else";
    }
    Foo::print = false;
}

// #include <iostream>
// #include <cstdlib>

// int main()
// {
//     std::cout << "Hello, notf!" << std::endl;
// }
