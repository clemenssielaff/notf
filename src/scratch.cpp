#include <iostream>
#include <string>

#include "utils/macro_overload.hpp"

struct Foo {
    std::string name;
};

constexpr void print_foos() {}

template <typename T, typename... ARGS,
          std::enable_if_t<!std::is_same<std::decay_t<T>, Foo>::value, std::nullptr_t> = nullptr>
constexpr void print_foos(T&&, ARGS&&... args)
{
    return print_foos(std::forward<ARGS>(args)...);
}

template <typename T, typename... ARGS,
          std::enable_if_t<std::is_same<std::decay_t<T>, Foo>::value, std::nullptr_t> = nullptr>
constexpr void print_foos(T&& foo, ARGS&&... args)
{
    //    foo.name = foo.name + " derbe";
    std::cout << foo.name << std::endl;
    return print_foos(std::forward<ARGS>(args)...);
}


#define _notf_variadic_capture(...) NOTF_OVERLOADED_MACRO(_notf_variadic_capture, __VA_ARGS__)
#define _notf_variadic_capture1(a) &#a=a
#define _notf_variadic_capture2(a, b) &a=a, &b=b
#define _notf_variadic_capture3(a, b, c) &#a=a, &#b=b, &#c=c
#define _notf_variadic_capture4(a, b, c, d) &#a=a, &#b=b, &#c=c, &#d=d
#define _notf_variadic_capture5(a, b, c, d, e) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e
#define _notf_variadic_capture6(a, b, c, d, e, f) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f
#define _notf_variadic_capture7(a, b, c, d, e, f, g) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g
#define _notf_variadic_capture8(a, b, c, d, e, f, g, h) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h
#define _notf_variadic_capture9(a, b, c, d, e, f, g, h, i) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i
#define _notf_variadic_capture10(a, b, c, d, e, f, g, h, i, j) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j
#define _notf_variadic_capture11(a, b, c, d, e, f, g, h, i, j, k) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k
#define _notf_variadic_capture12(a, b, c, d, e, f, g, h, i, j, k, l) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k, &#l=l
#define _notf_variadic_capture13(a, b, c, d, e, f, g, h, i, j, k, l, m) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k, &#l=l, &#m=m
#define _notf_variadic_capture14(a, b, c, d, e, f, g, h, i, j, k, l, m, n) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k, &#l=l, &#m=m, &#n=n
#define _notf_variadic_capture15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k, &#l=l, &#m=m, &#n=n, &#o=o
#define _notf_variadic_capture16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) &#a=a, &#b=b, &#c=c, &#d=d, &#e=e, &#f=f, &#g=g, &#h=h, &#i=i, &#j=j, &#k=k, &#l=l, &#m=m, &#n=n, &#o=o, &#p=p

#define create_foo(LAMBDA, ...) [_notf_variadic_capture(__VA_ARGS__)]()LAMBDA; print_foos(__VA_ARGS__)

struct Fobject {
    Fobject()
        : m_a{"a"}
        , m_b{"b"}
        , m_c{"c"}
    {
        auto whatevs = create_foo({
                                      std::cout << m_a.name << " / " << m_b.name << std::endl;
                                  }, m_a, m_b);
//        auto& _ref_m_a = m_a;
//        auto& _ref_m_b = m_b;
//        auto blub = [&m_a = m_a, &m_b = m_b](){std::cout << m_a.name << " / " << m_b.name << std::endl;};
    }

    Foo m_a;
    Foo m_b;
    Foo m_c;
};


//int notmain()
int main()
{
    Foo a = {"a"};
    Foo b = {"b"};
//    Foo c = {"c"};
//    Foo d = {"d"};
//    Foo e = {"d"};
//    Foo f = {"d"};
//    Foo g = {"d"};
//    Foo h = {"d"};
//    Foo i = {"d"};
//    Foo j = {"d"};
//    Foo k = {"d"};
//    Foo l = {"d"};
//    Foo m = {"d"};
//    Foo n = {"d"};
//    Foo o = {"d"};
//    Foo p = {"d"};

    int i = 0;

    auto blub = create_foo({
        std::cout << a.name << " / " << b.name << std::endl;
    }, a, b);
    blub();

    a.name = "a2";
    b.name = "b2";
    //    auto blub = [](auto&... args){ print_foos(args...); return [&](){std::cout << a.name << std::endl;} ;}(a);
    blub();

    return 0;
}
