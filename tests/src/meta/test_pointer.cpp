#include "catch2/catch.hpp"

#include "notf/meta/pointer.hpp"

NOTF_USING_NAMESPACE;

namespace  {
struct Foo {};
struct Bar : public Foo {};
}

SCENARIO("Pointer", "[meta][pointer]")
{
    SECTION("valid_ptr")
    {
        Bar a, b;
        auto raw = &a;
        auto valid = valid_ptr<Bar*>(&b);
        auto shared = std::make_shared<Bar>();
        auto unique = std::make_unique<Bar>();

        REQUIRE(raw_pointer(raw) == &a);
        REQUIRE(raw_pointer(valid) == &b);
        REQUIRE(raw_pointer(shared) == shared.get());
        REQUIRE(raw_pointer(unique) == unique.get());

        Bar* c = nullptr;
        REQUIRE_THROWS_AS(valid_ptr<Bar*>(c), valid_ptr<Bar*>::invalid_pointer_error);
        REQUIRE_THROWS_AS(valid_ptr<Bar*>(std::move(c)), valid_ptr<Bar*>::invalid_pointer_error);

        valid_ptr<Foo*> valid2(valid);
        REQUIRE(valid2 == valid);
    }
}
