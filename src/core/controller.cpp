#include "core/controller.hpp"

#include "core/screen_item.hpp"

namespace notf {

Controller::~Controller()
{
}

void Controller::_set_root_item(std::shared_ptr<ScreenItem> item)
{
    m_root_item = std::move(item);
    if (m_root_item) {
        m_root_item->_set_parent(shared_from_this());
    }
}

void Controller::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_root_item) {
        _get_widgets_at_item_pos(m_root_item.get(), local_pos, result);
    }
}

} // namespace notf
