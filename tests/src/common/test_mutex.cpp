#include "catch2/catch.hpp"

#include "notf/common/mutex.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("mutex", "[common][mutex]") {

    SECTION("Mutex") {
        Mutex mutex;
        NOTF_GUARD(std::lock_guard(mutex));
        REQUIRE(mutex.is_locked_by_this_thread());
    }

    SECTION("Recursive Mutex") {
        RecursiveMutex mutex;
        NOTF_GUARD(std::lock_guard(mutex));
        REQUIRE(mutex.is_locked_by_this_thread());
    }
}
