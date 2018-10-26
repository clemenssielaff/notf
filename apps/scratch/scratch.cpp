#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "notf/common/mutex.hpp"
#include "notf/meta/config.hpp"
#include "notf/meta/hash.hpp"
#include "notf/meta/types.hpp"
#include "notf/meta/pointer.hpp"

NOTF_USING_NAMESPACE;

struct Foo{};

int main()
{
    auto shared = std::make_shared<Foo>();
    std::weak_ptr<Foo> weak = shared;

    const Foo* raw_from_shared = shared.get();
    const Foo* raw_from_hack = raw_from_weak_ptr(weak);

    const bool is_same = (raw_from_shared == raw_from_hack);

    return 0;
}
