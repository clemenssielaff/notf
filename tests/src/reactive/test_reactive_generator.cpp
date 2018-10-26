#include "catch2/catch.hpp"

#include "notf/reactive/reactive_generator.hpp"

#include "test_reactive_utils.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("generator factory", "[reactive][generator]")
{
    SECTION("with all lambdas")
    {
        auto generator = make_generator(0,                           // initial state
                                        [](int& i) { ++i; },         // iterate
                                        [](int i) { return i < 3; }, // predicate
                                        [](int i) { return i * 2; }  // refine
        );
        auto sub = TestSubscriber();
        generator->subscribe(sub);
        for (auto i = 0; i < 10; ++i) {
            generator->publish();
        }
        REQUIRE(sub->values.size() == 3);
        REQUIRE(sub->values[0] == 0);
        REQUIRE(sub->values[1] == 2);
        REQUIRE(sub->values[2] == 4);
    }

//    SECTION("with iterator only")
//    {
//        auto generator = make_generator(0,                  // initial state
//                                        [](int& i) { ++i; } // iterate
//        );
//        auto sub = TestSubscriber();
//        generator->subscribe(sub);
//        for (auto i = 0; i < 3; ++i) {
//            generator->publish();
//        }
//        REQUIRE(sub->values.size() == 3);
//        REQUIRE(sub->values[0] == 0);
//        REQUIRE(sub->values[1] == 1);
//        REQUIRE(sub->values[2] == 2);
//    }

//    SECTION("with iterator only")
//    {
//        auto generator = make_generator(0,                  // initial state
//                                        [](int& i) { ++i; } // iterate
//        );
//        auto sub = TestSubscriber();
//        generator->subscribe(sub);
//        for (auto i = 0; i < 3; ++i) {
//            generator->publish();
//        }
//        REQUIRE(sub->values.size() == 3);
//        REQUIRE(sub->values[0] == 0);
//        REQUIRE(sub->values[1] == 1);
//        REQUIRE(sub->values[2] == 2);
//    }
}
