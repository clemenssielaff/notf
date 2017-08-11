#include "catch.hpp"

#include "dynamic/layout/flex_layout.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("A horizontal FlexLayout calculates its child aabr", "[dynamic][layout]")
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

        for (const auto cross_align : std::vector<FlexLayout::Alignment>{
                 FlexLayout::Alignment::START,
                 FlexLayout::Alignment::END,
                 FlexLayout::Alignment::CENTER}) {
            flexlayout->set_cross_alignment(cross_align);

            for (const auto main_align : std::vector<FlexLayout::Alignment>{
                     FlexLayout::Alignment::START,
                     FlexLayout::Alignment::END,
                     FlexLayout::Alignment::CENTER,
                     FlexLayout::Alignment::SPACE_BETWEEN,
                     FlexLayout::Alignment::SPACE_EQUAL,
                     FlexLayout::Alignment::SPACE_AROUND}) {
                flexlayout->set_alignment(main_align);

                for (const float spacing : std::vector<float>{
                         0.f,
                         10.f,
                     }) {
                    flexlayout->set_spacing(spacing);

                    for (const float padding : std::vector<float>{
                             0.f,
                             20.f,
                         }) {
                        flexlayout->set_padding(Padding::all(padding));

                        for (const auto direction : std::vector<FlexLayout::Direction>{
                                 FlexLayout::Direction::LEFT_TO_RIGHT,
                                 FlexLayout::Direction::RIGHT_TO_LEFT,
                             }) {
                            flexlayout->set_direction(direction);

                            THEN("the aabr of the FlexLayout is wide in the the main direction and narrow in the cross")
                            {
                                const Size2f layout_size = flexlayout->get_size();
                                REQUIRE(layout_size == Size2f(400, 200));
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("A vertical FlexLayout calculates its child aabr", "[dynamic][layout]")
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

        for (const auto cross_align : std::vector<FlexLayout::Alignment>{
                 FlexLayout::Alignment::START,
                 FlexLayout::Alignment::END,
                 FlexLayout::Alignment::CENTER}) {
            flexlayout->set_cross_alignment(cross_align);

            for (const auto main_align : std::vector<FlexLayout::Alignment>{
                     FlexLayout::Alignment::START,
                     FlexLayout::Alignment::END,
                     FlexLayout::Alignment::CENTER,
                     FlexLayout::Alignment::SPACE_BETWEEN,
                     FlexLayout::Alignment::SPACE_EQUAL,
                     FlexLayout::Alignment::SPACE_AROUND}) {
                flexlayout->set_alignment(main_align);

                for (const float spacing : std::vector<float>{
                         0.f,
                         10.f,
                     }) {
                    flexlayout->set_spacing(spacing);

                    for (const float padding : std::vector<float>{
                             0.f,
                             20.f,
                         }) {
                        flexlayout->set_padding(Padding::all(padding));

                        for (const auto direction : std::vector<FlexLayout::Direction>{
                                 FlexLayout::Direction::TOP_TO_BOTTOM,
                                 FlexLayout::Direction::BOTTOM_TO_TOP,
                             }) {
                            flexlayout->set_direction(direction);

                            THEN("the aabr of the FlexLayout is wide in the the main direction and narrow in the cross")
                            {
                                const Size2f layout_size = flexlayout->get_size();
                                REQUIRE(layout_size == Size2f(200, 400));
                            }
                        }
                    }
                }
            }
        }
    }
}
