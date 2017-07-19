#include "catch.hpp"

#include "dynamic/layout/overlayout.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("Overlayout", "[dynamic][layout][claim]")
{
    std::shared_ptr<Overlayout> overlayout = Overlayout::create();

    WHEN("an Overlayout has no explicit claim")
    {
        std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>(75, 14);
        overlayout->add_item(rect);

        THEN("it will at least require the combined minimum and preferred claims of its children")
        {
            REQUIRE(overlayout->get_claim().get_horizontal().get_min() == 75.f);
            REQUIRE(overlayout->get_claim().get_horizontal().get_preferred() == 75.f);
            REQUIRE(is_inf(overlayout->get_claim().get_horizontal().get_max()));
        }
    }
}
