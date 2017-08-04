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

        WHEN("you set the alignment to start")
        {
            flexlayout->set_alignment(FlexLayout::Alignment::START);

            WHEN("you set zero spacing")
            {
                flexlayout->set_spacing(0);

                WHEN("you set zero padding")
                {
                    flexlayout->set_padding(Padding::all(0));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(100));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(300));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(300));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(100));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(50));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(250));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(50));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(0));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(100));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(150));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }
                }

                WHEN("you set non-zero padding")
                {
                    flexlayout->set_padding(Padding::all(20));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(120));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(320));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(280));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(80));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(30));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(230));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(30));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(20));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(120));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(170));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }
                }
            }

            WHEN("you set non-zero spacing")
            {
                flexlayout->set_spacing(10);

                WHEN("you set zero padding")
                {
                    flexlayout->set_padding(Padding::all(0));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(110));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(320));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(300));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(90));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(30));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(240));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(30));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(0));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(110));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(170));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }
                }

                WHEN("you set non-zero padding")
                {
                    flexlayout->set_padding(Padding::all(20));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(130));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(340));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(280));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(70));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(10));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(220));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(10));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(20));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(130));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(190));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }
                }
            }
        }

        WHEN("you set the alignment to end")
        {
            flexlayout->set_alignment(FlexLayout::Alignment::END);

            WHEN("you set zero spacing")
            {
                flexlayout->set_spacing(0);

                WHEN("you set zero padding")
                {
                    flexlayout->set_padding(Padding::all(0));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(50));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(150));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(350));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(250));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(50));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(250));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(200));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(0));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(50));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(150));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }
                }

                WHEN("you set non-zero padding")
                {
                    flexlayout->set_padding(Padding::all(20));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(30));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(130));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(330));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(270));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(70));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(350, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(270));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(220));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(20));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(20));
                            REQUIRE(rect_trans.y() == approx(30));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(20));
                            REQUIRE(wideRect_trans.y() == approx(130));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(20));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 350));
                        }
                    }
                }
            }

            WHEN("you set non-zero spacing")
            {
                flexlayout->set_spacing(10);

                WHEN("you set zero padding")
                {
                    flexlayout->set_padding(Padding::all(0));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(30));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(140));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(350));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(270));
                            REQUIRE(rect_trans.y() == approx(300));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(60));
                            REQUIRE(wideRect_trans.y() == approx(350));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(270));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(210));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(0));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(0));
                            REQUIRE(rect_trans.y() == approx(30));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(0));
                            REQUIRE(wideRect_trans.y() == approx(140));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(0));
                            REQUIRE(highRect_trans.y() == approx(200));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(200, 370));
                        }
                    }
                }

                WHEN("you set non-zero padding")
                {
                    flexlayout->set_padding(Padding::all(20));

                    WHEN("you set the direction to left-to-right")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                        THEN("the widgets will be placed correctly")
                        {
                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(rect_trans.x() == approx(10));
                            REQUIRE(rect_trans.y() == approx(280));

                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(wideRect_trans.x() == approx(120));
                            REQUIRE(wideRect_trans.y() == approx(330));

                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                            REQUIRE(highRect_trans.x() == approx(330));
                            REQUIRE(highRect_trans.y() == approx(180));
                        }

                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                        {
                            const Size2f size = flexlayout->get_size();
                            REQUIRE(size == Size2f(370, 200));
                        }
                    }

                    WHEN("you set the direction to right-to-left")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

//                        THEN("the widgets will be placed correctly")
//                        {
//                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(rect_trans.x() == approx(280));
//                            REQUIRE(rect_trans.y() == approx(280));

//                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(wideRect_trans.x() == approx(70));
//                            REQUIRE(wideRect_trans.y() == approx(330));

//                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(highRect_trans.x() == approx(10));
//                            REQUIRE(highRect_trans.y() == approx(180));
//                        }

//                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
//                        {
//                            const Size2f size = flexlayout->get_size();
//                            REQUIRE(size == Size2f(370, 200));
//                        }
                    }

                    WHEN("you set the direction to top-to-bottom")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

//                        THEN("the widgets will be placed vertically at the top-left corner of the flexlayout")
//                        {
//                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(rect_trans.x() == approx(20));
//                            REQUIRE(rect_trans.y() == approx(280));

//                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(wideRect_trans.x() == approx(20));
//                            REQUIRE(wideRect_trans.y() == approx(220));

//                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(highRect_trans.x() == approx(20));
//                            REQUIRE(highRect_trans.y() == approx(10));
//                        }

//                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
//                        {
//                            const Size2f size = flexlayout->get_size();
//                            REQUIRE(size == Size2f(200, 370));
//                        }
                    }

                    WHEN("you set the direction to bottom-to-top")
                    {
                        flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

//                        THEN("the widgets will be placed vertically at the bottom-left corner of the flexlayout")
//                        {
//                            const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(rect_trans.x() == approx(20));
//                            REQUIRE(rect_trans.y() == approx(20));

//                            const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(wideRect_trans.x() == approx(20));
//                            REQUIRE(wideRect_trans.y() == approx(130));

//                            const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
//                            REQUIRE(highRect_trans.x() == approx(20));
//                            REQUIRE(highRect_trans.y() == approx(190));
//                        }

//                        THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
//                        {
//                            const Size2f size = flexlayout->get_size();
//                            REQUIRE(size == Size2f(200, 370));
//                        }
                    }
                }
            }
        }
    }
}
