#include "core/controller.hpp"

#include "common/log.hpp"
#include "core/screen_item.hpp"
#include "dynamic/layout/overlayout.hpp"

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
    if (!m_root_item) {
        log_trace << "Creating default empty Layout as root Item for Controller " << get_id();
        m_root_item = Overlayout::create();
        m_root_item->_set_parent(shared_from_this());
    }
}

void Controller::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (m_root_item) {
        Item::_get_widgets_at(m_root_item.get(), local_pos, result);
    }
}

void Controller::_cascade_render_layer(const RenderLayerPtr& render_layer)
{
    if(_has_own_render_layer() || render_layer == m_render_layer){
        return;
    }
    m_render_layer = render_layer;
    if (m_root_item) {
        Item::_cascade_render_layer(m_root_item.get(), render_layer);
    }
}

} // namespace notf
