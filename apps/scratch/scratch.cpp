#include <iostream>
#include <cstdlib>

#include "notf/meta/types.hpp"

using word_t = uint64_t;
static_assert(sizeof(void*) == sizeof(word_t));
static_assert(sizeof(std::size_t) == sizeof(word_t));

struct Foo {
    ~Foo() {
        std::cout << "Foo " << value << " deleted" << std::endl;
    }
    int value;
};

Foo* produce_lotta_foos(const int number)
{
    Foo* foos = new Foo[number]();
    for(int i = 0; i < number; ++i){
        foos[i].value = i + 3000;
    }
    return foos;
}

word_t hide_da_foos(const int number){
    return notf::to_number(produce_lotta_foos(number));
}

void delete_ma_foos(word_t as_word)
{
    delete[](std::launder(reinterpret_cast<Foo*>(as_word)));
}

int main()
{
    word_t some_hidden_foos = hide_da_foos(7);
    delete_ma_foos(some_hidden_foos);
    std::cout << "success" << std::endl;
    return 0;
}
