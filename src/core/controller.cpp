#include "core/controller.hpp"

#include "common/log.hpp"
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

void Controller::initialize()
{
    if (m_root_item) { // a Controller with a root Item has been initialized
        return;
    }
    _initialize();
    if(!m_root_item){
        log_critical << "Initialization of Controller " << get_id() << " did not set the Controller's root Item!";
    }
}

void Controller::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_root_item) {
        _get_widgets_at_item_pos(m_root_item.get(), local_pos, result);
    }
}

} // namespace notf
