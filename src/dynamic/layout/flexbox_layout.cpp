//#include "dynamic/layout/flexbox_layout.hpp"

//#include "common/log.hpp"
//#include "common/vector_utils.hpp"
//#include "core/widget.hpp"

//namespace signal {

//FlexboxLayout::FlexboxLayout(std::shared_ptr<Widget> owner,
//                                               DIRECTION direction, DIRECTION wrap_direction)
//    : LayoutComponent()
//    , m_items()
//    , m_direction(direction)
//    , m_wrap_direction(wrap_direction == DIRECTION::INVALID ? direction : wrap_direction)
//{
//}

//void FlexboxLayout::add_widget(std::shared_ptr<Widget> widget)
//{
//    m_items.emplace_back(std::move(widget));
////    update();
//}

//std::shared_ptr<Widget> FlexboxLayout::widget_at(const Vector2& local_pos)
//{
//    return {};
//}

//void FlexboxLayout::remove_widget(std::shared_ptr<Widget> widget)
//{
//    remove_one_unordered(m_items, widget);
////    update();
//}

////void FlexboxLayout::update()
////{
////}

//} // namespace signal
