#include "catch.hpp"

#include "dynamic/layout/flex_layout.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("A non-wrapping FlexLayout", "[dynamic][layout]")
{
    std::shared_ptr<FlexLayout> flexlayout = FlexLayout::create();

    std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>();
    rect->set_claim(Claim::fixed(100, 100));
    flexlayout->add_item(rect);

    std::shared_ptr<RectWidget> wideRect = std::make_shared<RectWidget>();
    wideRect->set_claim(Claim::fixed(200, 50));
    flexlayout->add_item(wideRect);

    std::shared_ptr<RectWidget> highRect = std::make_shared<RectWidget>();
    highRect->set_claim(Claim::fixed(50, 200));
    flexlayout->add_item(highRect);

    THEN("the aabrs of the FlexLayout are calculated and the children placed correctly")
    {
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
                         random_number<float>(1, 20),
                     }) {
                    flexlayout->set_spacing(spacing);

                    for (const float padding : std::vector<float>{
                             0.f,
                             random_number<float>(1, 30),
                         }) {
                        flexlayout->set_padding(Padding::all(padding));

                        const float used_main = 350 + (spacing * 2) + (padding * 2);
                        const float used_cross = 200 + (padding * 2);

                        for (const auto direction : std::vector<FlexLayout::Direction>{
                                 FlexLayout::Direction::LEFT_TO_RIGHT,
                                 FlexLayout::Direction::RIGHT_TO_LEFT,
                                 FlexLayout::Direction::TOP_TO_BOTTOM,
                                 FlexLayout::Direction::BOTTOM_TO_TOP,
                             }) {
                            flexlayout->set_direction(direction);

                            // random numbers from too small to accomodate to large enought with surplus
                            const float random_main  = random_number<float>(300, 500);
                            const float random_cross = random_number<float>(150, 250);
                            if(flexlayout->is_horizontal()){
                                flexlayout->set_claim(Claim::fixed(random_main, random_cross));
                            } else {
                                flexlayout->set_claim(Claim::fixed(random_cross, random_main));
                            }

                            // layout aabr
                            Size2f expected_layout_size;
                            if (flexlayout->is_horizontal()) {
                                expected_layout_size = Size2f(
                                    max(random_main, used_main),
                                    max(random_cross, used_cross));
                            }
                            else {
                                expected_layout_size = Size2f(
                                    max(random_cross, used_cross),
                                    max(random_main, used_main));
                            }
                            const Size2f layout_size = flexlayout->get_size();
                            REQUIRE(layout_size.is_approx(expected_layout_size, 0.1f));

                            // child aabr
                            float spacing_offset = 0;
                            if (main_align == FlexLayout::Alignment::SPACE_BETWEEN) {
                                spacing_offset = max(0, (random_main - used_main) / 2);
                            }
                            else if (main_align == FlexLayout::Alignment::SPACE_EQUAL) {
                                spacing_offset = max(0, (random_main - used_main) / 4);
                            }
                            else if (main_align == FlexLayout::Alignment::SPACE_AROUND) {
                                spacing_offset = max(0, (random_main - used_main) / 3);
                            }
                            Size2f expected_child_size;
                            if (flexlayout->is_horizontal()) {
                                expected_child_size = Size2f(
                                    350 + ((spacing + spacing_offset) * 2),
                                    200);
                            }
                            else {
                                expected_child_size = Size2f(
                                    200,
                                    350 + ((spacing + spacing_offset) * 2));
                            }
                            const Size2f child_size = flexlayout->get_content_aabr().get_size();
                            REQUIRE(child_size.is_approx(expected_child_size, 0.1f));
                        }
                    }
                }
            }
        }
    }
}
