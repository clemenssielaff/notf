#include "catch.hpp"

#include "dynamic/layout/overlayout.hpp"
#include "test_utils.hpp"
using namespace notf;

namespace { // anonymous

/** Fills an Overlayout with 2-10 items of random claims.
 * All arguments are out.
 * @param overlayout        The Overlayout to fill.
 * @param maxed_min         Maxed size of all minimal claim sizes.
 * @param maxed_preferred   Maxed size of all preferred claim sizes.
 */
void fill_overlayout(std::shared_ptr<Overlayout> overlayout, Size2f& maxed_min, Size2f& maxed_preferred, Size2f& maxed_max)
{
    maxed_min       = Size2f::wrongest();
    maxed_preferred = Size2f::wrongest();
    maxed_max       = Size2f::wrongest();
    for (int i = 0; i < random_number<int>(2, 10); ++i) {
        const bool add_before_change     = random_event(0.5);
        std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>();
        if (add_before_change) {
            overlayout->add_item(rect);
        }

        Size2f item_min       = random_size<float>(0, 10);
        Size2f item_preferred = item_min + random_size<float>(0, 10);
        Size2f item_max       = item_preferred + random_size<float>(0, 10);

        maxed_min.maxed(item_min);
        maxed_preferred.maxed(item_preferred);
        maxed_max.maxed(item_max);

        Claim claim;
        claim.set_min(std::move(item_min));
        claim.set_preferred(std::move(item_preferred));
        claim.set_max(std::move(item_max));
        rect->set_claim(claim);
        if (!add_before_change) {
            overlayout->add_item(rect);
        }
    }
}

} // namespace anonymous

SCENARIO("An Overlayout calculates its claim", "[dynamic][layout][claim]")
{
    std::shared_ptr<Overlayout> overlayout = Overlayout::create();

    WHEN("an Overlayout has no explicit claim")
    {
        WHEN("you add child items")
        {
            Size2f min, preferred, max;
            fill_overlayout(overlayout, min, preferred, max);

            THEN("it will use the largest the Claim values of its children for its implicit claim")
            {
                const Claim::Stretch& horizontal = overlayout->get_claim().get_horizontal();
                REQUIRE(horizontal.get_min() == approx(min.width));
                REQUIRE(horizontal.get_preferred() == approx(preferred.width));
                REQUIRE(horizontal.get_max() == approx(max.width));

                const Claim::Stretch& vertical = overlayout->get_claim().get_vertical();
                REQUIRE(vertical.get_min() == approx(min.height));
                REQUIRE(vertical.get_preferred() == approx(preferred.height));
                REQUIRE(vertical.get_max() == approx(max.height));

                WHEN("you add padding")
                {
                    Padding padding = random_padding(0, 10);
                    overlayout->set_padding(padding);

                    THEN("the layout will add the padding to its implicit claim")
                    {
                        REQUIRE(horizontal.get_min() == approx(min.width + padding.width()));
                        REQUIRE(horizontal.get_preferred() == approx(preferred.width + padding.width()));
                        REQUIRE(horizontal.get_max() == approx(max.width + padding.width()));

                        REQUIRE(vertical.get_min() == approx(min.height + padding.height()));
                        REQUIRE(vertical.get_preferred() == approx(preferred.height + padding.height()));
                        REQUIRE(vertical.get_max() == approx(max.height + padding.height()));
                    }
                }
            }

            WHEN("an explicit claim is added later on")
            {
                const Size2f layout_size = random_size<float>(2, 10);
                overlayout->set_claim(Claim::fixed(layout_size));

                THEN("it will use its own explict claim instead")
                {
                    const Claim::Stretch& horizontal = overlayout->get_claim().get_horizontal();
                    REQUIRE(horizontal.get_min() == approx(layout_size.width));
                    REQUIRE(horizontal.get_preferred() == approx(layout_size.width));
                    REQUIRE(horizontal.get_max() == approx(layout_size.width));

                    const Claim::Stretch& vertical = overlayout->get_claim().get_vertical();
                    REQUIRE(vertical.get_min() == approx(layout_size.height));
                    REQUIRE(vertical.get_preferred() == approx(layout_size.height));
                    REQUIRE(vertical.get_max() == approx(layout_size.height));
                }
            }
        }

        WHEN("you add padding without children")
        {
            Padding padding = random_padding(0, 10);
            overlayout->set_padding(padding);

            THEN("the layout will still have the default claim")
            {
                const Claim::Stretch& horizontal = overlayout->get_claim().get_horizontal();
                REQUIRE(horizontal.get_min() == approx(0));
                REQUIRE(horizontal.get_preferred() == approx(0));
                REQUIRE(is_inf(horizontal.get_max()));

                const Claim::Stretch& vertical = overlayout->get_claim().get_vertical();
                REQUIRE(vertical.get_min() == approx(0));
                REQUIRE(vertical.get_preferred() == approx(0));
                REQUIRE(is_inf(vertical.get_max()));
            }
        }
    }

    WHEN("an Overlayout has an explicit claim")
    {
        const Size2f layout_size = random_size<float>(2, 10);
        overlayout->set_claim(Claim::fixed(layout_size));

        THEN("adding children will not change its claim")
        {
            Size2f min, preferred, max;
            fill_overlayout(overlayout, min, preferred, max);

            const Claim::Stretch& horizontal = overlayout->get_claim().get_horizontal();
            REQUIRE(horizontal.get_min() == approx(layout_size.width));
            REQUIRE(horizontal.get_preferred() == approx(layout_size.width));
            REQUIRE(horizontal.get_max() == approx(layout_size.width));

            const Claim::Stretch& vertical = overlayout->get_claim().get_vertical();
            REQUIRE(vertical.get_min() == approx(layout_size.height));
            REQUIRE(vertical.get_preferred() == approx(layout_size.height));
            REQUIRE(vertical.get_max() == approx(layout_size.height));

            WHEN("you add padding")
            {
                Padding padding = random_padding(0, 10);
                overlayout->set_padding(padding);

                THEN("the layout will still not change its claim")
                {
                    REQUIRE(horizontal.get_min() == approx(layout_size.width));
                    REQUIRE(horizontal.get_preferred() == approx(layout_size.width));
                    REQUIRE(horizontal.get_max() == approx(layout_size.width));

                    REQUIRE(vertical.get_min() == approx(layout_size.height));
                    REQUIRE(vertical.get_preferred() == approx(layout_size.height));
                    REQUIRE(vertical.get_max() == approx(layout_size.height));
                }
            }
        }

        WHEN("you add padding without children")
        {
            Padding padding = random_padding(0, 10);
            overlayout->set_padding(padding);

            THEN("the layout will not change its claim")
            {
                const Claim::Stretch& horizontal = overlayout->get_claim().get_horizontal();
                REQUIRE(horizontal.get_min() == approx(layout_size.width));
                REQUIRE(horizontal.get_preferred() == approx(layout_size.width));
                REQUIRE(horizontal.get_max() == approx(layout_size.width));

                const Claim::Stretch& vertical = overlayout->get_claim().get_vertical();
                REQUIRE(vertical.get_min() == approx(layout_size.height));
                REQUIRE(vertical.get_preferred() == approx(layout_size.height));
                REQUIRE(vertical.get_max() == approx(layout_size.height));
            }
        }
    }
}

SCENARIO("An Overlayout aligns its children", "[dynamic][layout]")
{
    std::shared_ptr<Overlayout> overlayout = Overlayout::create();
    overlayout->set_claim(Claim::fixed(400, 400));

    WHEN("you add multiple widgets to an overlayout")
    {
        std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>();
        rect->set_claim(Claim::fixed(100, 100));
        overlayout->add_item(rect);

        std::shared_ptr<RectWidget> wideRect = std::make_shared<RectWidget>();
        wideRect->set_claim(Claim::fixed(200, 50));
        overlayout->add_item(wideRect);

        std::shared_ptr<RectWidget> highRect = std::make_shared<RectWidget>();
        highRect->set_claim(Claim::fixed(50, 200));
        overlayout->add_item(highRect);

        WHEN("you set no padding")
        {
            WHEN("you don't set an alignment")
            {
                THEN("the default top-left alignment is used")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(0));
                    REQUIRE(rect_trans.y() == approx(300));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(0));
                    REQUIRE(wideRect_trans.y() == approx(350));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(0));
                    REQUIRE(highRect_trans.y() == approx(200));
                }
            }

            WHEN("you set the alignment to horizontal and vertical center")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);

                THEN("the widgets will be placed at the center of the overlayout")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(150));
                    REQUIRE(rect_trans.y() == approx(150));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(100));
                    REQUIRE(wideRect_trans.y() == approx(175));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(175));
                    REQUIRE(highRect_trans.y() == approx(100));
                }
            }
            WHEN("you set the alignment bottom right")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);

                THEN("the widgets will be placed at the bottom-right corner of the overlayout")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(300));
                    REQUIRE(rect_trans.y() == approx(0));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(200));
                    REQUIRE(wideRect_trans.y() == approx(0));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(350));
                    REQUIRE(highRect_trans.y() == approx(0));
                }
            }
        }
        WHEN("you set nonzero padding on all sides")
        {
            overlayout->set_padding(Padding::all(20));

            WHEN("you don't set an alignment")
            {
                THEN("the default top-left alignment is used")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(20));
                    REQUIRE(rect_trans.y() == approx(280));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(20));
                    REQUIRE(wideRect_trans.y() == approx(330));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(20));
                    REQUIRE(highRect_trans.y() == approx(180));
                }
            }

            WHEN("you set the alignment to horizontal and vertical center")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);

                THEN("the widgets will be placed at the center of the overlayout")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(150));
                    REQUIRE(rect_trans.y() == approx(150));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(100));
                    REQUIRE(wideRect_trans.y() == approx(175));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(175));
                    REQUIRE(highRect_trans.y() == approx(100));
                }
            }
            WHEN("you set the alignment bottom right")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);

                THEN("the widgets will be placed at the bottom-right corner of the overlayout")
                {
                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(rect_trans.x() == approx(280));
                    REQUIRE(rect_trans.y() == approx(20));

                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(wideRect_trans.x() == approx(180));
                    REQUIRE(wideRect_trans.y() == approx(20));

                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                    REQUIRE(highRect_trans.x() == approx(330));
                    REQUIRE(highRect_trans.y() == approx(20));
                }
            }
        }
    }
}

SCENARIO("An Overlayout calculates its Aabr", "[dynamic][layout]")
{
    std::shared_ptr<Overlayout> overlayout = Overlayout::create();
    overlayout->set_claim(Claim::fixed(400, 400));

    WHEN("you add multiple widgets to an overlayout")
    {
        std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>();
        rect->set_claim(Claim::fixed(100, 100));
        overlayout->add_item(rect);

        std::shared_ptr<RectWidget> wideRect = std::make_shared<RectWidget>();
        wideRect->set_claim(Claim::fixed(200, 50));
        overlayout->add_item(wideRect);

        std::shared_ptr<RectWidget> highRect = std::make_shared<RectWidget>();
        highRect->set_claim(Claim::fixed(50, 200));
        overlayout->add_item(highRect);

        THEN("the size of the Overlayout's AABR remains the same, no matter the alignment")
        {
            WHEN("you don't set an alignment")
            {
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().get_size() == Size2f(200, 200));
            }
            WHEN("you set the alignment to horizontal and vertical center")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().get_size() == Size2f(200, 200));
            }
            WHEN("you set the alignment bottom right")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().get_size() == Size2f(200, 200));
            }

            WHEN("you add padding, it still stays the same")
            {
                overlayout->set_padding(Padding::all(20));
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().get_size() == Size2f(200, 200));
            }
        }

        THEN("the position of the Overlayout's AABR changes with the alignment")
        {
            WHEN("you don't set an alignment")
            {
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(0, 200));
            }
            WHEN("you set the alignment to horizontal and vertical center")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(100, 100));
            }
            WHEN("you set the alignment bottom right")
            {
                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);
                REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(200, 0));
            }

            WHEN("you add padding, the position changes accordingly")
            {
                overlayout->set_padding(Padding::all(20));

                WHEN("you don't set an alignment")
                {
                    REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(20, 180));
                }
                WHEN("you set the alignment to horizontal and vertical center")
                {
                    overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);
                    REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(100, 100));
                }
                WHEN("you set the alignment bottom right")
                {
                    overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);
                    REQUIRE(overlayout->get_aabr<Overlayout::Space::PARENT>().bottom_left() == Vector2f(180, 20));
                }
            }
        }
    }
}
