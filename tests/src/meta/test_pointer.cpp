#include "catch2/catch.hpp"

#include "notf/meta/hash.hpp"
#include "notf/meta/pointer.hpp"

NOTF_USING_NAMESPACE;

namespace {
struct Foo {};
struct Bar : public Foo {};
} // namespace

SCENARIO("Pointer", "[meta][pointer]") {
    SECTION("valid_ptr") {
        Bar a, b;
        auto raw = &a;
        auto valid = valid_ptr<Bar*>(&b);
        auto shared = std::make_shared<Bar>();
        auto unique = std::make_unique<Bar>();
        auto valid_shared = valid_ptr<std::shared_ptr<Bar>>(std::make_shared<Bar>());

        REQUIRE((raw_pointer(raw) == &a));
        REQUIRE((raw_pointer(valid) == &b));
        REQUIRE((raw_pointer(shared) == shared.get()));
        REQUIRE((raw_pointer(unique) == unique.get()));
        REQUIRE((raw_pointer(valid_shared) == valid_shared.get().get()));

        Bar* c = nullptr;
        REQUIRE_THROWS_AS(valid_ptr<Bar*>(c), valid_ptr<Bar*>::NotValidError);
        REQUIRE_THROWS_AS(valid_ptr<Bar*>(std::move(c)), valid_ptr<Bar*>::NotValidError);

        valid_ptr<Foo*> valid2(valid);
        REQUIRE((valid2 == valid));
    }

    SECTION("pointer_hash") {
        REQUIRE(pointer_hash<std::nullptr_t>()(nullptr) == 0);
        int i = 3;
        REQUIRE(pointer_hash<int*>()(&i) != 0);
    }
}
