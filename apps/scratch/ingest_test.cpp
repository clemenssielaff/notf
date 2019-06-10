#include "notf/meta/types.hpp"

#include <array>
#include <iostream>

struct matrix {
    matrix() {}
    matrix(matrix const&) { std::cout << "copy, "; }
    matrix(matrix&&) { std::cout << "move, "; }

    matrix& operator+=(notf::ingest<matrix> m) {
        std::cout << (m.is_movable() ? "move+=, " : "copy+=, ");
        return *this;
    }

    std::array<int, 65536> data;
};

inline matrix operator+(notf::ingest<matrix> x, notf::ingest<matrix> y) {
    return std::move(x.is_movable() ? x.force_move() += y : y.is_movable() ? y.force_move() += x : matrix(x) += y);
}

struct my_string {
    struct null_string {};
    my_string(const char* s) {
        if (s != 0)
            std::cout << "my_string (" << s << ")" << std::endl;
        else {
            std::cout << "my_string (0)" << std::endl;
            throw null_string();
        }
    }

    my_string(const my_string&) { std::cout << "my_string (copy)" << std::endl; }
    my_string(my_string&&) { std::cout << "my_string (move)" << std::endl; }
    ~my_string() { std::cout << "~my_string ()" << std::endl; }
};

int f(notf::ingest<my_string> s) {
    std::cout << "f (" << (s.is_movable() ? "rvalue" : "lvalue") << ")" << std::endl;
    return 0;
}

void g(int, int) { std::cout << "g ()" << std::endl; }

void m(notf::ingest<my_string> s) {
    std::cout << "m (" << (s.is_movable() ? "rvalue" : "lvalue") << ")" << std::endl;
    my_string c(s.move());
}

int main() {
    // Let's invoke all the possible permutations of lvalues/rvalues
    {
        matrix s1;
        matrix s2;
        matrix m1(matrix() + s1);
        std::cout << std::endl;
        matrix m2(matrix() + matrix());
        std::cout << std::endl;
        matrix m3(s1 + s2);
        std::cout << std::endl;
        matrix m4(s1 + matrix());
        std::cout << std::endl;

        // and a couple more
        matrix m5(matrix() + matrix() + matrix());
        std::cout << std::endl;
        matrix m6(s1 + matrix() + s2 + matrix());
        std::cout << std::endl;
    }

    // Test lvalue/rvalue distinction.
    //
    {
        std::cout << std::endl;
        my_string l("lvalue");
        const my_string& lr(l);

        std::cout << std::endl;
        f(l);
        std::cout << std::endl;
        f(lr);
        std::cout << std::endl;
        f(my_string("rvalue"));
        std::cout << std::endl;
        f(std::move(l));
        std::cout << std::endl;
    }

    // Test move.
    //
    {
        std::cout << std::endl;
        my_string l("lvalue");
        std::cout << std::endl;

        m(l);
        std::cout << std::endl;
        m(my_string("rvalue"));
        std::cout << std::endl;
    }

    // Test implicit conversion.
    //
    {
        std::cout << std::endl;
        f("implicit");
        std::cout << std::endl;
        g(f("implicit1"), 1);
        std::cout << std::endl;
        g(1, f("implicit2"));
        std::cout << std::endl;
        g(f("implicit1"), f("implicit2"));
        std::cout << std::endl;
    }

    // Test exception in implicit conversion.
    //
    try {
        f((const char*)0);
    }
    catch (const my_string::null_string&) {
    }
}