#include "catch.hpp"

#include "dynamic/layout/flex_layout.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("A FlexLayout places its children", "[dynamic][layout]")
{
    std::shared_ptr<FlexLayout> flexlayout = FlexLayout::create();
    flexlayout->set_claim(Claim::fixed(400, 400));

    WHEN("you add multiple widgets to an overlayout")
    {
        std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>();
        rect->set_claim(Claim::fixed(100, 100));
        flexlayout->add_item(rect);

        std::shared_ptr<RectWidget> wideRect = std::make_shared<RectWidget>();
        wideRect->set_claim(Claim::fixed(200, 50));
        flexlayout->add_item(wideRect);

        std::shared_ptr<RectWidget> highRect = std::make_shared<RectWidget>();
        highRect->set_claim(Claim::fixed(50, 200));
        flexlayout->add_item(highRect);

        WHEN("you set no padding")
        {
//            WHEN("you don't set an alignment")
//            {
//                THEN("the default top-left alignment is used")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(0));
//                    REQUIRE(rect_trans.y() == approx(300));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(100));
//                    REQUIRE(wideRect_trans.y() == approx(350));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(300));
//                    REQUIRE(highRect_trans.y() == approx(200));
//                }

//                THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
//                {
//                    const Size2f size = flexlayout->get_size();
//                    REQUIRE(size == Size2f(350, 200));
//                }
//            }

//            WHEN("you set the alignment to horizontal and vertical center")
//            {
//                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);

//                THEN("the widgets will be placed at the center of the overlayout")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(150));
//                    REQUIRE(rect_trans.y() == approx(150));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(100));
//                    REQUIRE(wideRect_trans.y() == approx(175));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(175));
//                    REQUIRE(highRect_trans.y() == approx(100));
//                }
//            }
//            WHEN("you set the alignment bottom right")
//            {
//                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);

//                THEN("the widgets will be placed at the bottom-right corner of the overlayout")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(300));
//                    REQUIRE(rect_trans.y() == approx(0));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(200));
//                    REQUIRE(wideRect_trans.y() == approx(0));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(350));
//                    REQUIRE(highRect_trans.y() == approx(0));
//                }
//            }
//        }
//        WHEN("you set nonzero padding on all sides")
//        {
//            overlayout->set_padding(Padding::all(20));

//            WHEN("you don't set an alignment")
//            {
//                THEN("the default top-left alignment is used")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(20));
//                    REQUIRE(rect_trans.y() == approx(280));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(20));
//                    REQUIRE(wideRect_trans.y() == approx(330));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(20));
//                    REQUIRE(highRect_trans.y() == approx(180));
//                }
//            }

//            WHEN("you set the alignment to horizontal and vertical center")
//            {
//                overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);

//                THEN("the widgets will be placed at the center of the overlayout")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(150));
//                    REQUIRE(rect_trans.y() == approx(150));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(100));
//                    REQUIRE(wideRect_trans.y() == approx(175));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(175));
//                    REQUIRE(highRect_trans.y() == approx(100));
//                }
//            }
//            WHEN("you set the alignment bottom right")
//            {
//                overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::BOTTOM);

//                THEN("the widgets will be placed at the bottom-right corner of the overlayout")
//                {
//                    const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(rect_trans.x() == approx(280));
//                    REQUIRE(rect_trans.y() == approx(20));

//                    const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(wideRect_trans.x() == approx(180));
//                    REQUIRE(wideRect_trans.y() == approx(20));

//                    const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                    REQUIRE(highRect_trans.x() == approx(330));
//                    REQUIRE(highRect_trans.y() == approx(20));
//                }
//            }
        }
    }
}
