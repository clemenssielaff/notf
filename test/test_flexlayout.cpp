#include "catch.hpp"

#include "dynamic/layout/flex_layout.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("A FlexLayout places its children without wrapping", "[dynamic][layout]")
{
    std::shared_ptr<FlexLayout> flexlayout = FlexLayout::create();
    flexlayout->set_claim(Claim::fixed(400, 400));

    WHEN("you add multiple widgets to a no-wrap overlayout")
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

        WHEN("you set the cross-alignment to start")
        {
            flexlayout->set_cross_alignment(FlexLayout::Alignment::START);

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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(290));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(290));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(230));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(10));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(120));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(180));
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

            WHEN("you set the alignment to center")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::CENTER);

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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(185));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(185));
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

            WHEN("you set the alignment to space-between")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_BETWEEN);

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
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
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
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
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
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
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
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
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
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
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
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-equal")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_EQUAL);

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
                                REQUIRE(rect_trans.x() == approx(12.5));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(337.5));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(287.5));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(12.5));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(287.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(12.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(12.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(187.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
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
                                REQUIRE(rect_trans.x() == approx(22.5));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(327.5));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(277.5));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(22.5));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(277.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(22.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(22.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(177.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
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
                                REQUIRE(rect_trans.x() == approx(7.5));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(342.5));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(292.5));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(7.5));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(292.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(7.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(7.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(192.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
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
                                REQUIRE(rect_trans.x() == approx(17.5));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(332.5));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(282.5));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(17.5));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(282.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(17.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(17.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(182.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-around")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_AROUND);

                WHEN("you set zero spacing")
                {
                    flexlayout->set_spacing(0);

                    WHEN("you set zero padding")
                    {
                        flexlayout->set_padding(Padding::all(0));

                        const float spacing = 50.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(300 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400 - spacing * 2, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(50 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(400 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(300 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(250 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(50 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(100 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(150 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = 10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80 - spacing * 3));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30 - spacing * 5));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(280 - spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(230 - spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(30 - spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(20 + spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(120 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(170 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
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
                                REQUIRE(rect_trans.x() == approx(5));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(345));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(295));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(350));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(5));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(295));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(0));
                                REQUIRE(rect_trans.y() == approx(5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(0));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(195));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = -10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(340 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(330));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(10 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(280 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(220 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(10 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20));
                                REQUIRE(rect_trans.y() == approx(20 + spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(20));
                                REQUIRE(wideRect_trans.y() == approx(130 + spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(190 + spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }
                    }
                }
            }
        }

        WHEN("you set the cross-alignment to end")
        {
            flexlayout->set_cross_alignment(FlexLayout::Alignment::END);

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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(300));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(50));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(250));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(100));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(230));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(120));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(110));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(90));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(240));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(110));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(340));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(10));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(220));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(130));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(150));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(50));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(250));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(200));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(50));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(150));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(270));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(220));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(30));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(130));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(140));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(60));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(270));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(210));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(30));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(140));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.x() == approx(290));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(290));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(230));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(10));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(120));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(180));
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

            WHEN("you set the alignment to center")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::CENTER);

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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
                                REQUIRE(highRect_trans.y() == approx(0));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
                                REQUIRE(highRect_trans.y() == approx(0));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(185));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
                                REQUIRE(highRect_trans.y() == approx(20));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(185));
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

            WHEN("you set the alignment to space-between")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_BETWEEN);

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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
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
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-equal")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_EQUAL);

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
                                REQUIRE(rect_trans.x() == approx(12.5));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(337.5));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(287.5));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(12.5));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(287.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(12.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(12.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(187.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
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
                                REQUIRE(rect_trans.x() == approx(22.5));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(327.5));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(277.5));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(22.5));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(277.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(22.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(22.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(177.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
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
                                REQUIRE(rect_trans.x() == approx(7.5));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(342.5));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(292.5));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(7.5));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(292.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(7.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(7.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(192.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
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
                                REQUIRE(rect_trans.x() == approx(17.5));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(332.5));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(282.5));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(17.5));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(282.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(17.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(17.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(182.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-around")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_AROUND);

                WHEN("you set zero spacing")
                {
                    flexlayout->set_spacing(0);

                    WHEN("you set zero padding")
                    {
                        flexlayout->set_padding(Padding::all(0));

                        const float spacing = 50.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(300 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400 - spacing * 2, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(50 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(400 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(300 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(250 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(50 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(100 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(150 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = 10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80 - spacing * 3));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30 - spacing * 5));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280 - spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(230 - spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(30 - spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20 + spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(120 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(170 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
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
                                REQUIRE(rect_trans.x() == approx(5));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(345));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(295));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(0));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(5));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(295));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(200));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(195));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = -10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(340 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(20));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(10 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(280 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(220 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(10 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(20 + spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(180));
                                REQUIRE(wideRect_trans.y() == approx(130 + spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(190 + spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }
                    }
                }
            }
        }

        WHEN("you set the cross-alignment to center")
        {
            flexlayout->set_cross_alignment(FlexLayout::Alignment::CENTER);

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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(300));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(50));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(250));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(100));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(230));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(120));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(110));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(90));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(240));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(110));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(340));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(10));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(220));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(130));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(150));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(50));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(250));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(200));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(50));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(150));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(270));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(220));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(30));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(130));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(140));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(60));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(270));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(210));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(30));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(140));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.x() == approx(290));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(290));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(230));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(20));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(10));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(120));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(180));
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

            WHEN("you set the alignment to center")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::CENTER);

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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(25));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(325));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.x() == approx(275));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(25));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(275));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(25));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(25));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(175));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(185));
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
                                REQUIRE(rect_trans.x() == approx(15));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(335));
                                REQUIRE(highRect_trans.y() == approx(100));
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
                                REQUIRE(rect_trans.x() == approx(285));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(15));
                                REQUIRE(highRect_trans.y() == approx(100));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(285));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(15));
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

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(15));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(185));
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

            WHEN("you set the alignment to space-between")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_BETWEEN);

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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(350));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(0));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(300));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(0));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(0));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(200));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 400));
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
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(330));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(20));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(360, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(20));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(180));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 360));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-equal")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_EQUAL);

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
                                REQUIRE(rect_trans.x() == approx(12.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(337.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(287.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(12.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(375, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(287.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(12.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(12.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(187.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 375));
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
                                REQUIRE(rect_trans.x() == approx(22.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(327.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(277.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(22.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(355, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(277.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(22.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(22.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(177.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 355));
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
                                REQUIRE(rect_trans.x() == approx(7.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(342.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(292.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(7.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(385, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(292.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(7.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(7.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(192.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 385));
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
                                REQUIRE(rect_trans.x() == approx(17.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(332.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(282.5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(17.5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(365, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(282.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(17.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(17.5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(182.5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 365));
                            }
                        }
                    }
                }
            }

            WHEN("you set the alignment to space-around")
            {
                flexlayout->set_alignment(FlexLayout::Alignment::SPACE_AROUND);

                WHEN("you set zero spacing")
                {
                    flexlayout->set_spacing(0);

                    WHEN("you set zero padding")
                    {
                        flexlayout->set_padding(Padding::all(0));

                        const float spacing = 50.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(300 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(400 - spacing * 2, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(300 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(50 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(400 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(300 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(250 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(50 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(100 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(150 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 400 - spacing * 2), 0.1f));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = 10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(120 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(320 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(80 - spacing * 3));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(30 - spacing * 5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280 - spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(230 - spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(30 - spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20 + spacing));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(120 + spacing * 3));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(170 + spacing * 5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
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
                                REQUIRE(rect_trans.x() == approx(5));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(125));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(345));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(295));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(75));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(5));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(390, 200));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(295));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(225));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(5));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(5));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(125));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(195));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size == Size2f(200, 390));
                            }
                        }
                    }

                    WHEN("you set non-zero padding")
                    {
                        flexlayout->set_padding(Padding::all(20));

                        const float spacing = -10.f / 6.f;

                        WHEN("you set the direction to left-to-right")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::LEFT_TO_RIGHT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(20 + spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(130 + spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(340 + spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to right-to-left")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::RIGHT_TO_LEFT);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(280 - spacing, 0.1f));
                                REQUIRE(rect_trans.y() == approx(150));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(70 - spacing * 3, 0.1f));
                                REQUIRE(wideRect_trans.y() == approx(175));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(10 - spacing * 5, 0.1f));
                                REQUIRE(highRect_trans.y() == approx(100));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(360 - spacing * 2, 200), 0.1f));
                            }
                        }

                        WHEN("you set the direction to top-to-bottom")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::TOP_TO_BOTTOM);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(280 - spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(220 - spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(10 - spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }

                        WHEN("you set the direction to bottom-to-top")
                        {
                            flexlayout->set_direction(FlexLayout::Direction::BOTTOM_TO_TOP);

                            THEN("the widgets will be placed correctly")
                            {
                                const Vector2f rect_trans = rect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(rect_trans.x() == approx(150));
                                REQUIRE(rect_trans.y() == approx(20 + spacing, 0.1f));

                                const Vector2f wideRect_trans = wideRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(wideRect_trans.x() == approx(100));
                                REQUIRE(wideRect_trans.y() == approx(130 + spacing * 3, 0.1f));

                                const Vector2f highRect_trans = highRect->get_xform<ScreenItem::Space::PARENT>().get_translation();
                                REQUIRE(highRect_trans.x() == approx(175));
                                REQUIRE(highRect_trans.y() == approx(190 + spacing * 5, 0.1f));
                            }

                            THEN("the size of the FlexLayout is the narrow bounding rect around the 3 items")
                            {
                                const Size2f size = flexlayout->get_size();
                                REQUIRE(size.is_approx(Size2f(200, 360 - spacing * 2), 0.1f));
                            }
                        }
                    }
                }
            }
        }
    }
}
